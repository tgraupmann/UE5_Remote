// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "IImageWrapper.h"
#include "IWebSocket.h"
#include "UE5_RemoteCharacter.generated.h"

class USceneCaptureComponent2D;

UCLASS(config=Game)
class AUE5_RemoteCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AUE5_RemoteCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void CreateRenderTarget(const int32 width, const int32 height);

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void SendRenderTexture();

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void StartWorkerSendRenderTexture();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	bool WaitForExit;

protected:

	void ProcessWebSocketMessage(const FString& MessageString);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	USceneCaptureComponent2D* CaptureComp;

	UTextureRenderTarget2D* RenderTarget;

	TArray<uint8> RenderTextureRawData;

	TSharedPtr<IImageWrapper> ImageWrapper;	

	TArray<TSharedPtr<IWebSocket>> WebSockets;

	int32 MaxRenderWebSockets;
	int32 IndexWebSocket; //cycle WebSockets

	bool InjectKeyW;
	bool InjectKeyA;
	bool InjectKeyS;
	bool InjectKeyD;
	bool InjectKeySpace;

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

class SendRenderTextureTask : public FNonAbandonableTask
{
public:
	SendRenderTextureTask(AUE5_RemoteCharacter* Actor);

	~SendRenderTextureTask();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(SendRenderTextureTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
private:
	AUE5_RemoteCharacter* Character;
	bool WaitForExit;
};
