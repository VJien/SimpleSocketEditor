// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/SSHud_SimpleSocket.h"
#include "SlateOptMacros.h"
#include "Widget/SEditorViewport_SimpleSocket.h"
#include "SEditorViewportToolBarMenu.h"
#include "SEditorViewportToolBarButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widget/SSocketEdit_SimpleSocket.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystem_SimpleSocket.h"
#include "SimpleSocketEditorCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Layout/SSplitter.h"



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSHud_SimpleSocket::Construct(const FArguments& InArgs)
{
	USubsystem_SimpleSocket* subsys = GEditor->GetEditorSubsystem<USubsystem_SimpleSocket>();
	if (subsys && subsys->CurrentMesh)
	{
		EditMesh = subsys->CurrentMesh;
	}
	else 
	{
		EditMesh = LoadObject<UStaticMesh>(NULL, TEXT("StaticMesh'/Engine/EngineMeshes/Cube.Cube'"), NULL, LOAD_None, NULL);
	}

	BindCommands();

	AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(24));

	ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[

			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			.Value(0.1f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				[
					SAssignNew(EditWidgetBox, SObjectPropertyEntryBox)
					.AllowedClass(UStaticMesh::StaticClass())
					.OnObjectChanged(this, &SSHud_SimpleSocket::OnSetObject)
					.ObjectPath(this, &SSHud_SimpleSocket::GetPathName)
					.ThumbnailPool(AssetThumbnailPool)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				[
					SNew(SBox)
				
				]
				
			]

			+ SSplitter::Slot()
			.Value(0.9f)
			[
				SNew(SSplitter)
				.Orientation(EOrientation::Orient_Horizontal)
				+ SSplitter::Slot()
				.Value(0.7f)
				[
					SAssignNew(ViewPort, SEditorViewport_SimpleSocket)
					.HUD(SharedThis(this))
					.PreviewMesh(EditMesh)
				]
				+ SSplitter::Slot()
				.Value(0.3f)
				[
					SAssignNew(SocketEditor, SSocketEdit)
					.StaticMesh(EditMesh)
					.HUD(SharedThis(this))
					.OnSocketSelectionChanged(this, &SSHud_SimpleSocket::SeletectSocketChanged)
				]
			]



			
			
		]
	];

	if (ViewPort.IsValid())
	{
		ViewPort.Get()->UpdatePreviewMesh(EditMesh);
	}
	if (SocketEditor.IsValid())
	{
		SocketEditor.Get()->SetNewMesh(EditMesh);
	}

	
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

UStaticMesh* SSHud_SimpleSocket::GetMesh() const
{
	return EditMesh;
}

UStaticMeshComponent* SSHud_SimpleSocket::GetMeshComponent() const
{
	return EditMeshComp;
}


TSharedPtr<SEditorViewport_SimpleSocket> SSHud_SimpleSocket::GetViewPort()
{
	return ViewPort;
}

void SSHud_SimpleSocket::SeletectSocketChanged()
{
	ViewPort.Get()->UpdateSelectedSocket(SocketEditor.Get()->GetSelectedSocket());
}

void SSHud_SimpleSocket::SetSelectedSocket(UStaticMeshSocket* InSelectedSocket)
{
	SocketEditor.Get()->SetSelectedSocket(InSelectedSocket);
	ViewPort.Get()->UpdateSelectedSocket(InSelectedSocket);
}

UStaticMeshSocket* SSHud_SimpleSocket::GetSelectedSocket() const
{
	return SocketEditor.IsValid()? SocketEditor.Get()->GetSelectedSocket(): nullptr;
}

void SSHud_SimpleSocket::DuplicateSelectedSocket()
{
	SocketEditor.Get()->DuplicateSelectedSocket();
}

void SSHud_SimpleSocket::DeleteSeletedSocket()
{
	SocketEditor.Get()->DeleteSelectedSocket();
}

void SSHud_SimpleSocket::RenameSeletedSocket()
{
	SocketEditor.Get()->RequestRenameSelectedSocket();
}


void SSHud_SimpleSocket::FocusSeleted()
{
	ViewPort.Get()->OnFocusViewportToSelection();
}

void SSHud_SimpleSocket::OnSetObject(const FAssetData& AssetData)
{
	if (UStaticMesh* newMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
	{
		EditMesh = newMesh;
		ViewPort.Get()->UpdatePreviewMesh(newMesh);
		SocketEditor.Get()->SetNewMesh(newMesh);


		USubsystem_SimpleSocket* subsys = GEditor->GetEditorSubsystem<USubsystem_SimpleSocket>();
		if (subsys)
		{
			subsys->CurrentMesh = newMesh;
		}
	}
	
}

FString SSHud_SimpleSocket::GetPathName()const
{
	return EditMesh ? EditMesh->GetPathName() : TEXT("");
}



void SSHud_SimpleSocket::BindCommands()
{
	const FSimpleSocketEditorCommands& Commands = FSimpleSocketEditorCommands::Get();

	CommandList = MakeShareable(new FUICommandList);


}
