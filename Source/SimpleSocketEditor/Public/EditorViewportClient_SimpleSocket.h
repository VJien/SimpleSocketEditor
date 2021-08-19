
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UnrealWidget.h"
#include "EditorViewportClient.h"
#include "Components.h"

class SSHud_SimpleSocket;
class FAdvancedPreviewScene;
class SEditorViewport_SimpleSocket;
class FCanvas;



/** Viewport Client for the preview viewport */
class FEditorViewportClient_SimpleSocket : public FEditorViewportClient, public TSharedFromThis<FEditorViewportClient_SimpleSocket>
{
public:
	FEditorViewportClient_SimpleSocket(TWeakPtr<SSHud_SimpleSocket> InHud, const TSharedRef<SEditorViewport_SimpleSocket>& InViewport, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene, UStaticMesh* InPreviewStaticMesh, UStaticMeshComponent* InPreviewStaticMeshComponent);
	~FEditorViewportClient_SimpleSocket();

	// FEditorViewportClient interface
	virtual void MouseMove(FViewport* Viewport, int32 x, int32 y) override;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false) override;
	virtual bool InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
	virtual void ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
	virtual void TrackingStopped() override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual void SetWidgetMode(FWidget::EWidgetMode NewMode) override;
	virtual bool CanSetWidgetMode(FWidget::EWidgetMode NewMode) const override;
	virtual bool CanCycleWidgetMode() const override;
	virtual FVector GetWidgetLocation() const override;
	virtual FMatrix GetWidgetCoordSystem() const override;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const override;
	// FEditorViewportClient interface ........




	void SetPreviewMesh(UStaticMesh* InStaticMesh, UStaticMeshComponent* InPreviewStaticMeshComponent, bool bResetCamera = true);
	void OnSocketSelectionChanged(UStaticMeshSocket* SelectedSocket);


private:
	bool IsCustomModeUsingWidget() const;
	void DrawAngles(FCanvas* Canvas, int32 XPos, int32 YPos, EAxisList::Type ManipAxis, FWidget::EWidgetMode MoveMode, const FRotator& Rotation, const FVector& Translation);
public:

	const FColor SocketColor_Default = FColor(255, 128, 128);
	const FColor SocketColor_Selected = FColor(0, 255, 0);

	FWidget::EWidgetMode WidgetMode;

	TWeakPtr<SSHud_SimpleSocket> HudPtr;
	TWeakPtr<SEditorViewport_SimpleSocket> ViewportPtr;


	UStaticMeshComponent* StaticMeshComponent;

	UStaticMesh* StaticMesh;
	FAdvancedPreviewScene* AdvancedPreviewScene;



	bool bManipulating;
};