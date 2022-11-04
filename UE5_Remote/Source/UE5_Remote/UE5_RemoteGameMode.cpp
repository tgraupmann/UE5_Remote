// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5_RemoteGameMode.h"
#include "UE5_RemoteCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE5_RemoteGameMode::AUE5_RemoteGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
