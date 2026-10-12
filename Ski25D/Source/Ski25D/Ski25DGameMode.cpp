// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ski25DGameMode.h"
#include "Ski25DCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASki25DGameMode::ASki25DGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
