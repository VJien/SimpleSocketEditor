#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "UObject/GCObject.h"
#include "SCommonEditorViewportToolbarBase.h"

class UStaticMesh;
class UStaticMeshComponent;
class SSHud_SimpleSocket;
class FEditorViewportClient_SimpleSocket;
class FAdvancedPreviewScene;

class SEditorViewport_SimpleSocket : public SEditorViewport, public FGCObject, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SEditorViewport_SimpleSocket) {}
		SLATE_ARGUMENT(UStaticMesh*, PreviewMesh)
		SLATE_ARGUMENT(TSharedPtr< SSHud_SimpleSocket >, HUD)
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	SEditorViewport_SimpleSocket();
	~SEditorViewport_SimpleSocket();


#pragma region SEditorViewport
	virtual void OnFocusViewportToSelection()override;
#pragma endregion SEditorViewport


#pragma region ICommonEditorViewportToolbarInfoProvider
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
#pragma endregion ICommonEditorViewportToolbarInfoProvider


#pragma region FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
#pragma endregion FGCObject



	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	void UpdatePreviewMesh(UStaticMesh* InStaticMesh, bool bResetCamera= true);
	void InitPrevewMesh(UStaticMesh* NewMesh);
	void UpdatePreviewSocketMeshes();

	void UpdateSelectedSocket(UStaticMeshSocket* InSelectedSocket);


public:
	
	//TSharedPtr<FEditorViewportClient> EditorViewportClient;
protected:
	/** SEditorViewport interface */

#pragma region SEditorViewport
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar()override;
#pragma endregion SEditorViewport

	TSharedPtr<class FAdvancedPreviewScene> AdvancedScene;


	UStaticMesh* PreviewMesh;
	UStaticMeshComponent* PreviewMeshComp;
	TArray<UStaticMeshComponent*> SocketPreviewMeshComponents;


	TSharedPtr<SSHud_SimpleSocket> HUD;
	TSharedPtr<FEditorViewportClient_SimpleSocket> EditorViewportClient;


};
