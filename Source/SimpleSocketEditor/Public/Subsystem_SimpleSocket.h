// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Subsystem_SimpleSocket.generated.h"

/**
 * 
 */
UCLASS()
class SIMPLESOCKETEDITOR_API USubsystem_SimpleSocket : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	UStaticMesh* CurrentMesh;
};
