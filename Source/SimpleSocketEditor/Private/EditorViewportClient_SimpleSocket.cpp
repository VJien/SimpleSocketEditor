#include "EditorViewportClient_SimpleSocket.h"
#include "Widget/SSHud_SimpleSocket.h"
#include "HitProxies.h"
#include "Engine/StaticMeshSocket.h"
#include "AdvancedPreviewScene.h"
#include "Widget/SEditorViewport_SimpleSocket.h"
#include "Utils.h"
#include "EditorModeManager.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"

#define LOCTEXT_NAMESPACE "FEditorViewportClient_SimpleSocket"

struct HSMESocketProxy_SimpleSocket : public HHitProxy
{
	DECLARE_HIT_PROXY();

	int32							SocketIndex;

	HSMESocketProxy_SimpleSocket(int32 InSocketIndex) :
		HHitProxy(HPP_UI),
		SocketIndex(InSocketIndex) {}
};
IMPLEMENT_HIT_PROXY(HSMESocketProxy_SimpleSocket, HHitProxy);




FEditorViewportClient_SimpleSocket::FEditorViewportClient_SimpleSocket(TWeakPtr<SSHud_SimpleSocket> InHud, const TSharedRef<SEditorViewport_SimpleSocket>& InViewport, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene, UStaticMesh* InPreviewStaticMesh, UStaticMeshComponent* InPreviewStaticMeshComponent)
	:FEditorViewportClient(nullptr, &InPreviewScene.Get(), StaticCastSharedRef<SEditorViewport>(InViewport)),
	 HudPtr(InHud),ViewportPtr(InViewport)
{
	SetViewMode(VMI_Lit);
	WidgetMode = FWidget::WM_None;
	SetPreviewMesh(InPreviewStaticMesh, InPreviewStaticMeshComponent);

	AdvancedPreviewScene = static_cast<FAdvancedPreviewScene*>(PreviewScene);

}

FEditorViewportClient_SimpleSocket::~FEditorViewportClient_SimpleSocket()
{

}

void FEditorViewportClient_SimpleSocket::MouseMove(FViewport* InViewport, int32 x, int32 y)
{
	FEditorViewportClient::MouseMove(InViewport, x, y);
}

bool FEditorViewportClient_SimpleSocket::InputKey(FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed /*= 1.f*/, bool bGamepad /*= false*/)
{
	bool bHandled = FEditorViewportClient::InputKey(InViewport, ControllerId, Key, Event, AmountDepressed, false);

	
	if (Key == EKeys::Delete)
	{
		if (HudPtr.Pin()->GetSelectedSocket())
		{
			HudPtr.Pin()->DeleteSeletedSocket();
		}
		
	}
	else if (Key == EKeys::F2)
	{
		if (HudPtr.Pin()->GetSelectedSocket())
		{
			HudPtr.Pin()->RenameSeletedSocket();
		}

	}
	else if (Key == EKeys::LeftMouseButton && Event == IE_DoubleClick)
	{
		if (HudPtr.Pin()->GetSelectedSocket())
		{
			HudPtr.Pin()->FocusSeleted();
		}

	}


	// Handle viewport screenshot.
	bHandled |= InputTakeScreenshot(InViewport, Key, Event);

	bHandled |= AdvancedPreviewScene->HandleInputKey(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);

	return bHandled;
}

bool FEditorViewportClient_SimpleSocket::InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples /*= 1*/, bool bGamepad /*= false*/)
{
	bool bResult = true;

	if (!bDisableInput)
	{
		bResult = AdvancedPreviewScene->HandleViewportInput(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
		if (bResult)
		{
			Invalidate();
		}
		else
		{
			bResult = FEditorViewportClient::InputAxis(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
		}
	}

	return bResult;
}

void FEditorViewportClient_SimpleSocket::ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);

	bool ClearSelectedSockets = true;
	bool ClearSelectedPrims = true;
	bool ClearSelectedEdges = true;

	if (HitProxy)
	{
		if (HitProxy->IsA(HSMESocketProxy_SimpleSocket::StaticGetType()))
		{
			HSMESocketProxy_SimpleSocket* SocketProxy = (HSMESocketProxy_SimpleSocket*)HitProxy;

			UStaticMeshSocket* Socket = NULL;

			if (SocketProxy->SocketIndex < StaticMesh->Sockets.Num())
			{
				Socket = StaticMesh->Sockets[SocketProxy->SocketIndex];
			}

			if (Socket)
			{
				HudPtr.Pin()->SetSelectedSocket(Socket);
			}

			ClearSelectedSockets = false;
		}

	}

	if (ClearSelectedSockets && HudPtr.Pin()->GetSelectedSocket())
	{
		HudPtr.Pin()->SetSelectedSocket(NULL);
	}


	Invalidate();
}

void FEditorViewportClient_SimpleSocket::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (!StaticMesh || !StaticMesh->RenderData || !StaticMesh->RenderData->LODResources.IsValidIndex(0))
	{
		return;
	}


	if (StaticMesh)
	{
		for (int32 i = 0; i < StaticMesh->Sockets.Num(); i++)
		{
			UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
			if (Socket)
			{
				FColor SocketColor = (Socket == HudPtr.Pin()->GetSelectedSocket()? SocketColor_Selected : SocketColor_Default);
				FMatrix SocketTM;
				Socket->GetSocketMatrix(SocketTM, StaticMeshComponent);
				PDI->SetHitProxy(new HSMESocketProxy_SimpleSocket(i));
				DrawWireDiamond(PDI, SocketTM, 5.f, SocketColor, SDPG_Foreground);
				PDI->SetHitProxy(NULL);
			}
		}
	}

	FUnrealEdUtils::DrawWidget(View, PDI, StaticMeshComponent->GetComponentTransform().ToMatrixWithScale(), 0, 0, EAxisList::All, EWidgetMovementMode::WMM_Translate, false);

}

void FEditorViewportClient_SimpleSocket::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	const int32 HalfX = Viewport->GetSizeXY().X / 2 / GetDPIScale();
	const int32 HalfY = Viewport->GetSizeXY().Y / 2 / GetDPIScale();

	for (int32 i = 0; i < StaticMesh->Sockets.Num(); i++)
	{
		UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
		if (Socket != NULL)
		{
			FMatrix SocketTM;
			Socket->GetSocketMatrix(SocketTM, StaticMeshComponent);
			const FVector SocketPos = SocketTM.GetOrigin();
			const FPlane proj = View.Project(SocketPos);
			if (proj.W > 0.f)
			{
				const int32 XPos = HalfX + (HalfX * proj.X);
				const int32 YPos = HalfY + (HalfY * (proj.Y * -1));
				FColor SocketColor = (Socket == HudPtr.Pin()->GetSelectedSocket() ? SocketColor_Selected : SocketColor_Default);

				FCanvasTextItem TextItem(FVector2D(XPos, YPos), FText::FromString(Socket->SocketName.ToString()), GEngine->GetSmallFont(), SocketColor);
				Canvas.DrawItem(TextItem);

				const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket();
				if (bManipulating && SelectedSocket == Socket)
				{
					//Figure out the text height
					FTextSizingParameters Parameters(GEngine->GetSmallFont(), 1.0f, 1.0f);
					UCanvas::CanvasStringSize(Parameters, *Socket->SocketName.ToString());
					int32 YL = FMath::TruncToInt(Parameters.DrawYL);

					DrawAngles(&Canvas, XPos, YPos + YL,
						Widget->GetCurrentAxis(),
						GetWidgetMode(),
						Socket->RelativeRotation,
						Socket->RelativeLocation);
				}
			}
		}
	}
	
}

void FEditorViewportClient_SimpleSocket::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

bool FEditorViewportClient_SimpleSocket::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	bool bHandled = FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);

	if (!bHandled && bManipulating)
	{
		if (CurrentAxis != EAxisList::None)
		{
			UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket();

			if (SelectedSocket)
			{
				FProperty* ChangedProperty = NULL;
				const FWidget::EWidgetMode MoveMode = GetWidgetMode();

				if (MoveMode == FWidget::WM_Rotate)
				{
					ChangedProperty = FindFProperty<FProperty>(UStaticMeshSocket::StaticClass(), "RelativeRotation");
					SelectedSocket->PreEditChange(ChangedProperty);
					FRotator CurrentRot = SelectedSocket->RelativeRotation;
					FRotator SocketWinding, SocketRotRemainder;
					CurrentRot.GetWindingAndRemainder(SocketWinding, SocketRotRemainder);

					const FQuat ActorQ = SocketRotRemainder.Quaternion();
					const FQuat DeltaQ = Rot.Quaternion();
					const FQuat ResultQ = DeltaQ * ActorQ;
					const FRotator NewSocketRotRem = FRotator(ResultQ);
					FRotator DeltaRot = NewSocketRotRem - SocketRotRemainder;
					DeltaRot.Normalize();

					SelectedSocket->RelativeRotation += DeltaRot;
					SelectedSocket->RelativeRotation = SelectedSocket->RelativeRotation.Clamp();

				}
				else if (MoveMode == FWidget::WM_Translate)
				{
					ChangedProperty = FindFProperty<FProperty>(UStaticMeshSocket::StaticClass(), "RelativeLocation");
					SelectedSocket->PreEditChange(ChangedProperty);

					//FRotationMatrix SocketRotTM( SelectedSocket->RelativeRotation );
					//FVector SocketMove = SocketRotTM.TransformVector( Drag );

					SelectedSocket->RelativeLocation += Drag;
				}
				if (ChangedProperty)
				{
					FPropertyChangedEvent PropertyChangedEvent(ChangedProperty);
					SelectedSocket->PostEditChangeProperty(PropertyChangedEvent);
				}
				HudPtr.Pin()->GetMesh()->MarkPackageDirty();
			}
		}

		Invalidate();
		bHandled = true;
	}

	return bHandled;

}

void FEditorViewportClient_SimpleSocket::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge)
{
	const bool bTrackingHandledExternally = ModeTools->StartTracking(this, Viewport);

	if (!bManipulating && bIsDraggingWidget && !bTrackingHandledExternally)
	{
		Widget->SetSnapEnabled(true);
		const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket();
		if (SelectedSocket)
		{
			FText TransText;
			if (GetWidgetMode() == FWidget::WM_Rotate)
			{
				TransText = LOCTEXT("FStaticMeshEditorViewportClient_RotateSocket", "Rotate Socket");
			}
			else if (GetWidgetMode() == FWidget::WM_Translate)
			{
				if (InInputState.IsLeftMouseButtonPressed() && (Widget->GetCurrentAxis() & EAxisList::XYZ))
				{
					const bool bAltDown = InInputState.IsAltButtonPressed();
					if (bAltDown)
					{
						// Rather than moving/rotating the selected socket, copy it and move the copy instead
						HudPtr.Pin()->DuplicateSelectedSocket();
					}
				}

				TransText = LOCTEXT("FStaticMeshEditorViewportClient_TranslateSocket", "Translate Socket");
			}

			if (!TransText.IsEmpty())
			{
				GEditor->BeginTransaction(TransText);
			}
		}


		bManipulating = true;
	}


}

void FEditorViewportClient_SimpleSocket::TrackingStopped()
{
	const bool bTrackingHandledExternally = ModeTools->EndTracking(this, Viewport);

	if (bManipulating && !bTrackingHandledExternally)
	{
		bManipulating = false;
		GEditor->EndTransaction();
	}
}

FWidget::EWidgetMode FEditorViewportClient_SimpleSocket::GetWidgetMode() const
{
	if (IsCustomModeUsingWidget())
	{
		return ModeTools->GetWidgetMode();
	}
	else if (HudPtr.Pin()->GetSelectedSocket())
	{
		return WidgetMode;
	}

	return FWidget::WM_Max;
}

void FEditorViewportClient_SimpleSocket::SetWidgetMode(FWidget::EWidgetMode NewMode)
{
	if (IsCustomModeUsingWidget())
	{
		ModeTools->SetWidgetMode(NewMode);
	}
	else
	{
		WidgetMode = NewMode;
	}

	Invalidate();
}

bool FEditorViewportClient_SimpleSocket::CanSetWidgetMode(FWidget::EWidgetMode NewMode) const
{
	if (!Widget->IsDragging())
	{
		if (IsCustomModeUsingWidget())
		{
			return ModeTools->UsesTransformWidget(NewMode);
		}

		else if (NewMode != FWidget::WM_Scale)	// Sockets don't support scaling
		{
			const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket();
			if (SelectedSocket)
			{
				return true;
			}
		}
	}
	return false;
}

bool FEditorViewportClient_SimpleSocket::CanCycleWidgetMode() const
{
	if (!Widget->IsDragging())
	{
		const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket();

		if ((SelectedSocket || IsCustomModeUsingWidget()))
		{
			return true;
		}
	}
	return false;
}

FVector FEditorViewportClient_SimpleSocket::GetWidgetLocation() const
{
	if (IsCustomModeUsingWidget())
	{
		return ModeTools->GetWidgetLocation();
	}
	else if (const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket())
	{
		FMatrix SocketTM;
		SelectedSocket->GetSocketMatrix(SocketTM, StaticMeshComponent);

		return SocketTM.GetOrigin();
	}


	return FVector::ZeroVector;
}

FMatrix FEditorViewportClient_SimpleSocket::GetWidgetCoordSystem() const
{
	if (IsCustomModeUsingWidget())
	{
		return ModeTools->GetCustomInputCoordinateSystem();
	}
	if (const UStaticMeshSocket* SelectedSocket = HudPtr.Pin()->GetSelectedSocket())
	{
		return FRotationMatrix(SelectedSocket->RelativeRotation);
	}
	return FMatrix::Identity;
}

ECoordSystem FEditorViewportClient_SimpleSocket::GetWidgetCoordSystemSpace() const
{
	if (IsCustomModeUsingWidget())
	{
		return ModeTools->GetCoordSystem();
	}

	return COORD_Local;
}

void FEditorViewportClient_SimpleSocket::SetPreviewMesh(UStaticMesh* InStaticMesh, UStaticMeshComponent* InPreviewStaticMeshComponent, bool bResetCamera /*= true*/)
{
	StaticMesh = InStaticMesh;
	StaticMeshComponent = InPreviewStaticMeshComponent;

}

bool FEditorViewportClient_SimpleSocket::IsCustomModeUsingWidget() const
{
	const FWidget::EWidgetMode ToolsWidgetMode = ModeTools->GetWidgetMode();
	const bool bDisplayToolWidget = ModeTools->GetShowWidget();

	return bDisplayToolWidget && ToolsWidgetMode != FWidget::EWidgetMode::WM_None;
}


void FEditorViewportClient_SimpleSocket::DrawAngles(FCanvas* Canvas, int32 XPos, int32 YPos, EAxisList::Type ManipAxis, FWidget::EWidgetMode MoveMode, const FRotator& Rotation, const FVector& Translation)
{
	FString OutputString(TEXT(""));
	if (MoveMode == FWidget::WM_Rotate && Rotation.IsZero() == false)
	{
		//Only one value moves at a time
		const FVector EulerAngles = Rotation.Euler();
		if (ManipAxis == EAxisList::X)
		{
			OutputString += FString::Printf(TEXT("Roll: %0.2f"), EulerAngles.X);
		}
		else if (ManipAxis == EAxisList::Y)
		{
			OutputString += FString::Printf(TEXT("Pitch: %0.2f"), EulerAngles.Y);
		}
		else if (ManipAxis == EAxisList::Z)
		{
			OutputString += FString::Printf(TEXT("Yaw: %0.2f"), EulerAngles.Z);
		}
	}
	else if (MoveMode == FWidget::WM_Translate && Translation.IsZero() == false)
	{
		//Only one value moves at a time
		if (ManipAxis == EAxisList::X)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.X);
		}
		else if (ManipAxis == EAxisList::Y)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.Y);
		}
		else if (ManipAxis == EAxisList::Z)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.Z);
		}
	}

	if (OutputString.Len() > 0)
	{
		FCanvasTextItem TextItem(FVector2D(XPos, YPos), FText::FromString(OutputString), GEngine->GetSmallFont(), FLinearColor::White);
		Canvas->DrawItem(TextItem);
	}
}

void FEditorViewportClient_SimpleSocket::OnSocketSelectionChanged(UStaticMeshSocket* SelectedSocket)
{
	if (SelectedSocket)
	{	
		if (WidgetMode == FWidget::WM_None || WidgetMode == FWidget::WM_Scale)
		{
			WidgetMode = FWidget::WM_Translate;
		}
	}

	Invalidate();
}

#undef LOCTEXT_NAMESPACE 