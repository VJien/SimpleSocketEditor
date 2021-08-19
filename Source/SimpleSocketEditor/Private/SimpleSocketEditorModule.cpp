// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleSocketEditorModule.h"
#include "SimpleSocketEditorStyle.h"
#include "SimpleSocketEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widget/SEditorViewport_SimpleSocket.h"
#include "Widget/SSHud_SimpleSocket.h"
#include "Framework/Commands/GenericCommands.h"
#include "LevelEditor.h"

static const FName SimpleSocketEditorTabName("SimpleSocketEditor");

#define LOCTEXT_NAMESPACE "FSimpleSocketEditorModule"

void FSimpleSocketEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FSimpleSocketEditorStyle::Initialize();
	FSimpleSocketEditorStyle::ReloadTextures();

	FSimpleSocketEditorCommands::Register();
	

	MapActions();





	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSimpleSocketEditorModule::RegisterMenus));


	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SimpleSocketEditorTabName, FOnSpawnTab::CreateRaw(this, &FSimpleSocketEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("SocketEditorTitil", "Socket Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

TSharedRef<SDockTab> FSimpleSocketEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
//			SNew(STextBlock)
//			.Text(FText::FromString("Nice!"))
			SNew(SSHud_SimpleSocket)
		];


}



void FSimpleSocketEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSimpleSocketEditorStyle::Shutdown();

	FSimpleSocketEditorCommands::Unregister();
}

void FSimpleSocketEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SimpleSocketEditorTabName);
}

void FSimpleSocketEditorModule::MapActions()
{
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSimpleSocketEditorCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FSimpleSocketEditorModule::PluginButtonClicked),
		FCanExecuteAction());



}

void FSimpleSocketEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FSimpleSocketEditorCommands::Get().PluginAction, PluginCommands);
		}
	}




	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");



	//¹¤¾ßÀ¸
	TSharedPtr<FExtender> ToolBarExtender = MakeShareable(new FExtender());
	ToolBarExtender->AddToolBarExtension("File",
		EExtensionHook::Before, PluginCommands, FToolBarExtensionDelegate::CreateLambda(
			[this](FToolBarBuilder& Builder)
			{
				Builder.AddToolBarButton(FSimpleSocketEditorCommands::Get().PluginAction);
			}
		));

	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolBarExtender);
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleSocketEditorModule, SimpleSocketEditor)