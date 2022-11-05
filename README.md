# UE5_Remote

POC - Video Streaming with UE5

## Components

### WebSocket Server

The `WebSocket Server` relays streaming binary images to clients.

* Dependency: [websockets/ws](https://github.com/websockets/ws)

* `WebSocket Server` [Server/app.js](Server/app.js)

```
cd Server
npm install
node app.js
```

### HTML5 Client

The HTML5 client uses a WebSocket client to receive binary images and display in an img element.

WebSocket clients can send player input to the `Unreal Remote Host` by using WebSocket messages. Clients can inject keyboard and mouse input into the `Unreal Remote Host`.

![image_1](images/image_1.png)

### UE5 Remote Host

The [Unreal Remote Host](UE5_Remote/UE5_Remote.uproject) is an `UE5` project that uses a render texture and a WebSocket client to send binary image data to clients. Clients display the binary images. The UE5 project extends the third person perspective game template.

## Unit Tests

* [Server/sendImage.js](Server/sendImage.js) sends an image to the `WebSocket Server` that the client displays.

![image_2](images/image_2.png)

## Change Log

* Added [UE5_Remote/Content/UI/BPWidget_UI.uasset](UE5_Remote/Content/UI/BPWidget_UI.uasset) - BP Widget

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h)

Add RenderTarget include:

```C++
#include "Engine/CanvasRenderTarget2D.h"
```

Add BP Function:

```C++
 UFUNCTION(BlueprintCallable, Category = "Capture")
 UTextureRenderTarget2D* CreateRenderTarget(const int32 width, const int32 height);
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

Implement function:

```C++
UTextureRenderTarget2D* AUE5_RemoteCharacter::CreateRenderTarget(const int32 width, const int32 height)
{
 UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();

 RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

 RenderTarget->InitAutoFormat(width, height);
 RenderTarget->UpdateResourceImmediate(true);

 return RenderTarget;
}
```

* Updated [UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset](UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset) - Event Graph

* BeginPlay: calls `InitUI` and `SetupRenderTexture` custom events.

* InitUI: Loads UI BP Widget and saves to `Widget` variable.

* SetupRenderTexture: Creates `UTextureRenderTarget2D` and saves to `RenderTexture` variable.

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h)

Add class prototype:

```C++
class USceneCaptureComponent2D;
```

Add protected property for camera capture component:

```C++
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
USceneCaptureComponent2D* CaptureComp;
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

Include SceneCaptureComponent2D:

```C++
#include "Components/SceneCaptureComponent2D.h"
```

Attach Capture Component to the Camera Boom:

```C++
CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComp"));
if (CaptureComp)
{
  CaptureComp->SetupAttachment(CameraBoom);
}
```

* The BP Viewport will show the new capture component - [UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset](UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset)

![image_3](images/image_3.png)

* The texture target of the CaptureComponent has not been set yet.

![image_4](images/image_4.png)

* Create [UE5_Remote/Content/Materials/RenderTarget2D_Capture.uasset](UE5_Remote/Content/Materials/RenderTarget2D_Capture.uasset) render target at 480x270. This is 1/4 of 1080p by design.

* Set the Texture Target to use the `RenderTarget2D_Capture` RenderTarget.

![image_5](images/image_5.png)

* Set the `Capture Source` to `Final Color (LDR) in RGB`.

![image_6](images/image_6.png)

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h)

Add BP Functions:

```C++
UFUNCTION(BlueprintCallable, Category = "Capture")
bool GetRawData(UTextureRenderTarget2D* TexRT, TArray<uint8>& RawData);

UFUNCTION(BlueprintCallable, Category = "Capture")
void SendRenderTexture(UTextureRenderTarget2D* TextureRenderTarget);
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

Add implementation for getting raw data bytes from render texture.

```C++
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
```

Add headers:

```C++
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RenderUtils.h"
```

* Add `ImageWrapper`, `RenderCore`, and `RHI` modules to [UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs](UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs)

```C#
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ImageWrapper", "RenderCore", "RHI" });
```

* Regenerate the Visual Studio project files after a module change.

Include Websockets headers:

```C++
#include "WebSocketsModule.h"
#include "IWebSocket.h"
```

* Add `WebSockets` module to [UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs](UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs)

```C#
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ImageWrapper", "RenderCore", "RHI", "WebSockets" });
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h)

* Add include for `IWebSocket`

```C++
#include "IWebSocket.h"
```

* Add WebSocket protected data member.

```C++
TSharedPtr<IWebSocket> WebSocket;
```

* Override `BeginPlay` and `EndPlay`:

```C++
virtual void BeginPlay() override;

virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

* Implement `BeginPlay`:

```C++
void AUE5_RemoteCharacter::BeginPlay()
{
 Super::BeginPlay();

 if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
 {
  FModuleManager::Get().LoadModule("WebSockets");
 }

 WebSocket = FWebSocketsModule::Get().CreateWebSocket("ws://localhost:8080");

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

 WebSocket->OnMessage().AddLambda([](const FString& MessageString)
  {
   GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, MessageString);
  });

 WebSocket->Connect();
}
```

* Implement `EndPlay`:

```C++
void AUE5_RemoteCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 if (WebSocket->IsConnected())
 {
  WebSocket->Close();
 }

 Super::EndPlay(EndPlayReason);
}
```

* Implement `SendRenderTexture`:

```C++
void AUE5_RemoteCharacter::SendRenderTexture(UTextureRenderTarget2D* TextureRenderTarget)
{
 //UE_LOG(LogTemp, Log, TEXT("Client sending over WebSocket"));

 if (WebSocket->IsConnected() && TextureRenderTarget)
 {
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

   //TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP); //crash probably too large
   //TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG); // 10 FPS Max
   TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG); // 30 FPS

   ImageWrapper->SetRaw(RawData.GetData(), RawData.GetAllocatedSize(), Size.X, Size.Y, ERGBFormat::BGRA, 8);

   //const TArray64<uint8>& ImageData = ImageWrapper->GetCompressed(100); //largest size
   const TArray64<uint8>& ImageData = ImageWrapper->GetCompressed(0); //smallest size

   WebSocket->Send((void*)ImageData.GetData(), ImageData.GetAllocatedSize(), true);
  }
 }
 else
 {
  UE_LOG(LogTemp, Log, TEXT("HTTP module not available!"));
 }
}
```

## Blueprints

* Updated [UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset](UE5_Remote/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset) - Event Graph

* The `SetupRenderTexture` custom event sets up the RenderTexture called from the `EventPlay` event.

![image_7](images/image_7.png)

* The `SendRenderTextureToWebSocket` custom event sends image data limited by the `RefreshRate` variable.

![image_8](images/image_8.png)

* The `EventTick` calls the custom event.

![image_9](images/image_9.png)

## WebSocket Input

* Add `Json` module to [UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs](UE5_Remote/Source/UE5_Remote/UE5_Remote.Build.cs)

```C#
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ImageWrapper", "RenderCore", "RHI", "WebSockets", "Json" });
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

Add includes for `Json`:

```C++
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
```

Update OnMessage with `Json` parsing:

```C++
WebSocket->OnMessage().AddLambda([](const FString& MessageString)
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
     }
     else if (InputString.Equals("keydown"))
     {
      FString Key;
      if (JsonObject->TryGetStringField("key", Key))
      {
       if (Key.Equals("w"))
       {
       }
       else if (Key.Equals("a"))
       {
       }
       else if (Key.Equals("s"))
       {
       }
       else if (Key.Equals("d"))
       {
       }
       else if (Key.Equals("space"))
       {
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
       }
       else if (Key.Equals("a"))
       {
       }
       else if (Key.Equals("s"))
       {
       }
       else if (Key.Equals("d"))
       {
       }
       else if (Key.Equals("space"))
       {
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
```

* Turn on RunInBackground so when the browser has focus the game still reacts quickly to input and rendering.

Go to `Edit->Editor Preferences->Miscellaneous` and disable the `Use Less CPU When in Background` option.

![image_10](images/image_10.png)

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.h)

Add protected data members that will be toggled from the WASD key socket events.

```C++
bool InjectKeyW;
bool InjectKeyA;
bool InjectKeyS;
bool InjectKeyD;
```

This also will need an update tick to inject the input.

```C++
virtual void Tick(float DeltaTime) override;
```

* Updated Source: [UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp](UE5_Remote/Source/UE5_Remote/UE5_RemoteCharacter.cpp)

Initialize the data members and turn on the tick event.

```C++
AUE5_RemoteCharacter::AUE5_RemoteCharacter()
{
 InjectKeyW = false;
 InjectKeyA = false;
 InjectKeyS = false;
 InjectKeyD = false;

 // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
 PrimaryActorTick.bCanEverTick = true;
```

Add a tick event to inject input:

```C++
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
```

Toggle WASD key injection from the socket message:

```C++
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
        Jump();
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
```

Add mouse input by sending values to the mouse input actions from the socket message.

```C++
if (InputString.Equals("mouse"))
{
  int32 X = JsonObject->GetIntegerField("x");
  int32 Y = JsonObject->GetIntegerField("y");

  AddControllerYawInput(X);
  AddControllerPitchInput(Y);
}
```

## Support

Support is available on Discord, you can reach me at `Tim Graupmann#0611`.
