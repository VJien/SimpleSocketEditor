// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "PropertyCustomizationHelpers.h"

class UStaticMesh;
class UStaticMeshComponent;
class SEditorViewport_SimpleSocket;
class SObjectPropertyEntryBox;
class FAssetThumbnailPool;
class SSocketEdit;
class UStaticMeshSocket;

/**
 * 
 */
class SIMPLESOCKETEDITOR_API SSHud_SimpleSocket : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSHud_SimpleSocket)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	UStaticMesh* GetMesh() const;
	UStaticMeshComponent* GetMeshComponent() const;

	TSharedPtr<SEditorViewport_SimpleSocket> GetViewPort();

	void SeletectSocketChanged();
	void SetSelectedSocket(UStaticMeshSocket* InSelectedSocket);
	UStaticMeshSocket* GetSelectedSocket() const;
	void DuplicateSelectedSocket();
	void DeleteSeletedSocket();
	void RenameSeletedSocket();

	void FocusSeleted();

protected:
	void OnSetObject(const FAssetData& AssetData);
	FString GetPathName()const;

	void BindCommands();
	
protected:
	UStaticMesh* EditMesh;
	UStaticMeshComponent* EditMeshComp;

	TSharedPtr<SEditorViewport_SimpleSocket> ViewPort;
	TSharedPtr<SSocketEdit> SocketEditor;
	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool;
	TSharedPtr<SObjectPropertyEntryBox> EditWidgetBox;

	TSharedPtr<SWidget> ToolBar;

	TSharedPtr<class FUICommandList> CommandList;

};
