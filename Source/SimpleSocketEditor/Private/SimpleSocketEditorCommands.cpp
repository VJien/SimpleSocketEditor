// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleSocketEditorCommands.h"

#define LOCTEXT_NAMESPACE "FSimpleSocketEditorModule"

void FSimpleSocketEditorCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Simple Socket Editor", "Open Simple Socket Editor action", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
