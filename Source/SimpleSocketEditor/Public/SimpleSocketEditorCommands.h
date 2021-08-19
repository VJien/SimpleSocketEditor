// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SimpleSocketEditorStyle.h"

class FSimpleSocketEditorCommands : public TCommands<FSimpleSocketEditorCommands>
{
public:

	FSimpleSocketEditorCommands()
		: TCommands<FSimpleSocketEditorCommands>(TEXT("SimpleSocketEditor"), NSLOCTEXT("Contexts", "SimpleSocketEditor", "SimpleSocketEditor Plugin"), NAME_None, FSimpleSocketEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
