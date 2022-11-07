// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5_RemoteCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "ImageUtils.h"
#include "IImageWrapperModule.h"
#include "RenderUtils.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Misc/DateTime.h"
#include "GenericPlatform/GenericPlatformProcess.h"

//////////////////////////////////////////////////////////////////////////
// AUE5_RemoteCharacter

AUE5_RemoteCharacter::AUE5_RemoteCharacter()
{
	WaitForExit = true;

	RenderTarget = nullptr;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	//ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP); //crash probably too large
	//ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG); // 10 FPS Max
	ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG); // 30 FPS

	RenderTextureRawData.AddUninitialized(480 * 270 * 4);

	MaxRenderWebSockets = 2;
	IndexWebSocket = 0;

	InjectKeyW = false;
	InjectKeyA = false;
	InjectKeyS = false;
	InjectKeyD = false;
	InjectKeySpace = false;

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComp"));
	if (CaptureComp)
	{
		CaptureComp->SetupAttachment(CameraBoom);
	}

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUE5_RemoteCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AUE5_RemoteCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AUE5_RemoteCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AUE5_RemoteCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AUE5_RemoteCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AUE5_RemoteCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AUE5_RemoteCharacter::TouchStopped);
}

void AUE5_RemoteCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AUE5_RemoteCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AUE5_RemoteCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AUE5_RemoteCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AUE5_RemoteCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AUE5_RemoteCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AUE5_RemoteCharacter::CreateRenderTarget(const int32 width, const int32 height)
{
	RenderTarget = NewObject<UTextureRenderTarget2D>();

	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	RenderTarget->InitAutoFormat(width, height);
	RenderTarget->UpdateResourceImmediate(true);

	RenderTarget->TargetGamma = 2.2f;

	CaptureComp->TextureTarget = RenderTarget;
}

void AUE5_RemoteCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	TSharedPtr<IWebSocket> WebSocket = FWebSocketsModule::Get().CreateWebSocket("ws://localhost:8080/?type=input");

	WebSocket->OnConnected().AddLambda([]()
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Successfully connected");
		});

	WebSocket->OnConnectionError().AddLambda([](const FString& Error)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Error);
		});

	WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, bWasClean ? FColor::Green : FColor::Red, "Connection closed " + Reason);
		});

	WebSocket->OnMessage().AddLambda([this](const FString& MessageString)
		{
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(MessageString);
			if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
			{
				FString InputString;
				if (JsonObject->TryGetStringField("input", InputString))
				{
					if (InputString.Equals("mouse"))
					{
						int32 X = JsonObject->GetIntegerField("x");
						int32 Y = JsonObject->GetIntegerField("y");

						AddControllerYawInput(X);
						AddControllerPitchInput(Y);
					}
					else if (InputString.Equals("keydown"))
					{
						FString Key;
						if (JsonObject->TryGetStringField("key", Key))
						{
							if (Key.Equals("w"))
							{
								InjectKeyW = true;
							}
							else if (Key.Equals("a"))
							{
								InjectKeyA = true;
							}
							else if (Key.Equals("s"))
							{
								InjectKeyS = true;
							}
							else if (Key.Equals("d"))
							{
								InjectKeyD = true;
							}
							else if (Key.Equals("space"))
							{
								if (!InjectKeySpace)
								{
									InjectKeySpace = true;
									Jump();
								}
							}
						}
					}
					else if (InputString.Equals("keyup"))
					{
						FString Key;
						if (JsonObject->TryGetStringField("key", Key))
						{
							if (Key.Equals("w"))
							{
								InjectKeyW = false;
							}
							else if (Key.Equals("a"))
							{
								InjectKeyA = false;
							}
							else if (Key.Equals("s"))
							{
								InjectKeyS = false;
							}
							else if (Key.Equals("d"))
							{
								InjectKeyD = false;
							}
							else if (Key.Equals("space"))
							{
								InjectKeySpace = false;
								StopJumping();
							}
						}
					}
				}
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "OnMessage: " + MessageString);
			}
		});

	WebSocket->Connect();
	WebSockets.Add(WebSocket);

	for (int index = 0; index < MaxRenderWebSockets; ++index)
	{
		TSharedPtr<IWebSocket> WebSocketAlt = FWebSocketsModule::Get().CreateWebSocket("ws://localhost:8080/?type=render");
		WebSocketAlt->Connect();
		WebSockets.Add(WebSocketAlt);
	}
}

void AUE5_RemoteCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WaitForExit = false;

	for (int index = 0; index < WebSockets.Num(); ++index)
	{
		TSharedPtr<IWebSocket> WebSocket = WebSockets[index];
		if (WebSocket->IsConnected())
		{
			WebSocket->Close();
		}
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AUE5_RemoteCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InjectKeyW)
	{
		MoveForward(1);
	}
	else if (InjectKeyS)
	{
		MoveForward(-1);
	}
	
	if (InjectKeyA)
	{
		MoveRight(-1);
	}
	else if (InjectKeyD)
	{
		MoveRight(1);
	}
}

void AUE5_RemoteCharacter::SendRenderTexture()
{
	//UE_LOG(LogTemp, Log, TEXT("Client sending over WebSocket"));
	// The zero index is the input WebSocket
	TSharedPtr<IWebSocket> WebSocket = WebSockets[IndexWebSocket + 1];
	IndexWebSocket = (IndexWebSocket + 1) % MaxRenderWebSockets; // cycle between web sockets

	if (WebSocket->IsConnected() && RenderTarget)
	{
		FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

		bool bSuccess = RenderTargetResource->ReadPixelsPtr((FColor*)RenderTextureRawData.GetData());
		if (bSuccess)
		{
			ImageWrapper->SetRaw(RenderTextureRawData.GetData(), RenderTextureRawData.GetAllocatedSize(), 480, 270, ERGBFormat::BGRA, 8);
			const TArray64<uint8>& ImageData = ImageWrapper->GetCompressed(0); //smallest size
			WebSocket->Send((void*)ImageData.GetData(), ImageData.GetAllocatedSize(), true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("SendRenderTexture: WebSocket is not connected!"));
	}
}

void AUE5_RemoteCharacter::StartWorkerSendRenderTexture()
{
	(new FAutoDeleteAsyncTask<SendRenderTextureTask>(this))->StartBackgroundTask();
}

SendRenderTextureTask::SendRenderTextureTask(AUE5_RemoteCharacter* Actor)
{
	//UE_LOG(LogTemp, Log, TEXT("SendRenderTextureTask()"));
	Character = Actor;
	WaitForExit = true;
}

SendRenderTextureTask::~SendRenderTextureTask()
{
	//UE_LOG(LogTemp, Log, TEXT("~SendRenderTextureTask()"));
	WaitForExit = false;
}

void SendRenderTextureTask::DoWork()
{
	const float RefreshRate = 1 / 60.0f * 0.4;
	//FDateTime LogTimer = FDateTime::Now();
	//LogTimer += 1000;
	bool notStarted = true;
	while (WaitForExit && Character->WaitForExit)
	{
		if (notStarted)
		{
			notStarted = false;
			AsyncTask(ENamedThreads::GameThread, [this, &notStarted]() {
				// code to execute on game thread here
				Character->SendRenderTexture();
				notStarted = true;
				//UE_LOG(LogTemp, Log, TEXT("SendRenderTexture completed!"));
			});
		}
		/*
		else
		{
			if (LogTimer < FDateTime::Now())
			{
				LogTimer = FDateTime::Now() + 1000;
				UE_LOG(LogTemp, Log, TEXT("SendRenderTexture: is busy!"));
			}
		}
		*/

		FWindowsPlatformProcess::Sleep(RefreshRate);
	}
}
