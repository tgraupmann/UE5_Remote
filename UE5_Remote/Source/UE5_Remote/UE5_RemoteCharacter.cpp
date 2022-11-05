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
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RenderUtils.h"

//////////////////////////////////////////////////////////////////////////
// AUE5_RemoteCharacter

AUE5_RemoteCharacter::AUE5_RemoteCharacter()
{
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

UTextureRenderTarget2D* AUE5_RemoteCharacter::CreateRenderTarget(const int32 width, const int32 height)
{
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();

	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	RenderTarget->InitAutoFormat(width, height);
	RenderTarget->UpdateResourceImmediate(true);

	return RenderTarget;
}

// From: C:\Program Files\Epic Games\UE_4.21\Engine\Source\Runtime\Engine\Private\ImageUtils.cpp
bool AUE5_RemoteCharacter::GetRawData(UTextureRenderTarget2D* TexRT, TArray<uint8>& RawData)
{
	FRenderTarget* RenderTarget = TexRT->GameThread_GetRenderTargetResource();
	EPixelFormat Format = TexRT->GetFormat();

	int32 ImageBytes = CalculateImageBytes(TexRT->SizeX, TexRT->SizeY, 0, Format);
	RawData.AddUninitialized(ImageBytes);
	bool bReadSuccess = false;
	switch (Format)
	{
	case PF_FloatRGBA:
	{
		TArray<FFloat16Color> FloatColors;
		bReadSuccess = RenderTarget->ReadFloat16Pixels(FloatColors);
		FMemory::Memcpy(RawData.GetData(), FloatColors.GetData(), ImageBytes);
	}
	break;
	case PF_B8G8R8A8:
		bReadSuccess = RenderTarget->ReadPixelsPtr((FColor*)RawData.GetData());
		break;
	}
	if (bReadSuccess == false)
	{
		RawData.Empty();
	}
	return bReadSuccess;
}

void AUE5_RemoteCharacter::SendRenderTexture(UTextureRenderTarget2D* TextureRenderTarget)
{
	UE_LOG(LogTemp, Log, TEXT("Client sending over WebSocket"));

	// Switch from HTTP to WebSocket

	//FHttpModule* Http = &FHttpModule::Get();
	if (TextureRenderTarget /*&&
		Http && Http->IsHttpEnabled() */)
	{
		//FHttpRequestPtr Request = Http->CreateRequest();

		if (TextureRenderTarget->GetFormat() != PF_B8G8R8A8)
		{
			EPixelFormat TextureFormat = TextureRenderTarget->GetFormat();
			UE_LOG(LogTemp, Log, TEXT("Render Target is not in the expected format should be 'PF_B8G8R8A8' instead found '%s'!"),
				GetPixelFormatString(TextureFormat));
		}
		else
		{
			check(TextureRenderTarget != nullptr);
			FRenderTarget* RenderTarget = TextureRenderTarget->GameThread_GetRenderTargetResource();
			FIntPoint Size = RenderTarget->GetSizeXY();

			TArray<uint8> RawData;
			bool bSuccess = GetRawData(TextureRenderTarget, RawData);

			IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

			TSharedPtr<IImageWrapper> PNGImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

			PNGImageWrapper->SetRaw(RawData.GetData(), RawData.GetAllocatedSize(), Size.X, Size.Y, ERGBFormat::BGRA, 8);

			const TArray64<uint8>& PNGData = PNGImageWrapper->GetCompressed(100);

			//Ar.Serialize((void*)PNGData.GetData(), PNGData.GetAllocatedSize());

			if (bSuccess)
			{
				//Request->SetURL("ws://localhost:8080");

				/*
				Request->SetContent(data);

				Request->OnProcessRequestComplete().BindUObject(this, &ARCCharacter::OnUploadRequestComplete);
				if (!Request->ProcessRequest())
				{
					UE_LOG(LogTemp, Log, TEXT("HTTP failed to process request!"));
				}
				*/
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("HTTP module not available!"));
	}
}
