#include "Widget/SEditorViewport_SimpleSocket.h"
#include "PreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "Subsystem_SimpleSocket.h"
#include "AdvancedPreviewScene.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "EditorViewportClient_SimpleSocket.h"
#include "Widget/SSHud_SimpleSocket.h"


void SEditorViewport_SimpleSocket::Construct(const FArguments& InArgs)
{
	PreviewMesh = InArgs._PreviewMesh;
	HUD = InArgs._HUD;

	
	FAdvancedPreviewScene::ConstructionValues ConstructValues;	//预览场景参数
	ConstructValues.SetCreatePhysicsScene(false);				//关闭物理场景
	ConstructValues.ShouldSimulatePhysics(false);				//关闭物理模拟
	ConstructValues.LightBrightness = 3;						//设置光照强度
	ConstructValues.SkyBrightness = 1;							//设置天空光强度
	ConstructValues.bEditor = true;

	AdvancedScene = MakeShareable(new FAdvancedPreviewScene(ConstructValues));

	//添加天空光组件
	USkyLightComponent* Skylight = NewObject<USkyLightComponent>();
	AdvancedScene->AddComponent(Skylight, FTransform::Identity);

	//设置地面
	AdvancedScene->SetFloorVisibility(false);		//显示地面，默认就是显示
	AdvancedScene->SetFloorOffset(100.f);		//设置地面高度偏移，正数是往下偏移
	//设置方向光
	AdvancedScene->DirectionalLight->SetMobility(EComponentMobility::Movable);
	AdvancedScene->DirectionalLight->CastShadows = true;				//启用阴影
	AdvancedScene->DirectionalLight->CastStaticShadows = true;
	AdvancedScene->DirectionalLight->CastDynamicShadows = true;
	AdvancedScene->DirectionalLight->SetIntensity(3);



	PreviewMeshComp = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	ERHIFeatureLevel::Type FeatureLevel = GEditor->PreviewPlatform.GetEffectivePreviewFeatureLevel();
	if (FeatureLevel <= ERHIFeatureLevel::ES3_1)
	{
		PreviewMeshComp->SetMobility(EComponentMobility::Static);
	}
	USubsystem_SimpleSocket* subsys = GEditor->GetEditorSubsystem<USubsystem_SimpleSocket>();
	UStaticMesh* mesh = nullptr;
	if (subsys && subsys->CurrentMesh)
	{
		mesh = subsys->CurrentMesh;
	}
	else
	{
		mesh = LoadObject<UStaticMesh>(NULL, TEXT("StaticMesh'/Engine/EngineMeshes/Cube.Cube'"), NULL, LOAD_None, NULL);
	}
	
	InitPrevewMesh(mesh);
	


	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &SEditorViewport_SimpleSocket::OnObjectPropertyChanged);


	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SEditorViewport_SimpleSocket::SEditorViewport_SimpleSocket()
{

}

SEditorViewport_SimpleSocket::~SEditorViewport_SimpleSocket()
{
	PreviewMeshComp = nullptr;
	PreviewMesh = nullptr;
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);

	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = NULL;
	}
}
void SEditorViewport_SimpleSocket::OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!ensure(ObjectBeingModified))
	{
		return;
	}
	if (PreviewMeshComp)
	{
		bool bShouldUpdatePreviewSocketMeshes = (ObjectBeingModified == PreviewMeshComp->GetStaticMesh());
		if (!bShouldUpdatePreviewSocketMeshes && PreviewMeshComp->GetStaticMesh())
		{
			const int32 SocketCount = PreviewMeshComp->GetStaticMesh()->Sockets.Num();
			for (int32 i = 0; i < SocketCount; ++i)
			{
				if (ObjectBeingModified == PreviewMeshComp->GetStaticMesh()->Sockets[i])
				{
					bShouldUpdatePreviewSocketMeshes = true;
					break;
				}
			}
		}

		if (bShouldUpdatePreviewSocketMeshes)
		{
			UpdatePreviewSocketMeshes();
			//RefreshViewport();
		}
	}

}
void SEditorViewport_SimpleSocket::OnFocusViewportToSelection()
{

	UStaticMeshSocket* SelectedSocket = HUD->GetSelectedSocket();
	if (SelectedSocket && PreviewMeshComp)
	{
		FTransform SocketTransform;
		SelectedSocket->GetSocketTransform(SocketTransform, PreviewMeshComp);

		const FVector Extent(30.0f);

		const FVector Origin = SocketTransform.GetLocation();
		const FBox Box(Origin - Extent, Origin + Extent);

		EditorViewportClient->FocusViewportOnBox(Box);
		return;
	}




	if (EditorViewportClient.IsValid())
	{
		if (PreviewMeshComp)
		{

			EditorViewportClient->FocusViewportOnBox(PreviewMeshComp->Bounds.GetBox());
			return;		
		}
		FBox Box(ForceInit);
		EditorViewportClient->FocusViewportOnBox(Box);
	
	}

	

}

TSharedRef<class SEditorViewport> SEditorViewport_SimpleSocket::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SEditorViewport_SimpleSocket::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SEditorViewport_SimpleSocket::OnFloatingButtonClicked()
{

}

void SEditorViewport_SimpleSocket::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PreviewMesh);
	Collector.AddReferencedObject(PreviewMeshComp);
	Collector.AddReferencedObjects(SocketPreviewMeshComponents);
}

void SEditorViewport_SimpleSocket::UpdatePreviewMesh(UStaticMesh* InStaticMesh, bool bResetCamera/*= true*/)
{
	PreviewMesh = InStaticMesh;
	{
		const int32 SocketedComponentCount = SocketPreviewMeshComponents.Num();
		for (int32 i = 0; i < SocketedComponentCount; ++i)
		{
			UStaticMeshComponent* SocketPreviewMeshComponent = SocketPreviewMeshComponents[i];
			if (SocketPreviewMeshComponent)
			{
				AdvancedScene->RemoveComponent(SocketPreviewMeshComponent);
			}
		}
		SocketPreviewMeshComponents.Empty();
	}

	if (PreviewMeshComp)
	{
		AdvancedScene->RemoveComponent(PreviewMeshComp);
		PreviewMeshComp = NULL;
	}

	PreviewMeshComp = NewObject<UStaticMeshComponent>();
	ERHIFeatureLevel::Type FeatureLevel = GEditor->PreviewPlatform.GetEffectivePreviewFeatureLevel();
	if (FeatureLevel <= ERHIFeatureLevel::ES3_1)
	{
		PreviewMeshComp->SetMobility(EComponentMobility::Static);
	}
	PreviewMeshComp->SetStaticMesh(InStaticMesh);

	AdvancedScene->AddComponent(PreviewMeshComp, FTransform::Identity);

	const int32 SocketCount = InStaticMesh->Sockets.Num();
	SocketPreviewMeshComponents.Reserve(SocketCount);
	for (int32 i = 0; i < SocketCount; ++i)
	{
		UStaticMeshSocket* Socket = InStaticMesh->Sockets[i];

		UStaticMeshComponent* SocketPreviewMeshComponent = NULL;
		if (Socket && Socket->PreviewStaticMesh)
		{
			SocketPreviewMeshComponent = NewObject<UStaticMeshComponent>();
			SocketPreviewMeshComponent->SetStaticMesh(Socket->PreviewStaticMesh);
			SocketPreviewMeshComponent->AttachToComponent(PreviewMeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket->SocketName);
			SocketPreviewMeshComponents.Add(SocketPreviewMeshComponent);
			AdvancedScene->AddComponent(SocketPreviewMeshComponent, FTransform::Identity);
		}
	}

	EditorViewportClient->SetPreviewMesh(InStaticMesh, PreviewMeshComp, bResetCamera);


	PreviewMeshComp->PushSelectionToProxy();



	OnFocusViewportToSelection();
	
	


}

void SEditorViewport_SimpleSocket::InitPrevewMesh(UStaticMesh* NewMesh)
{
	FComponentReregisterContext ReregisterContext(PreviewMeshComp);
	PreviewMeshComp->SetStaticMesh(NewMesh);

	FTransform Transform = FTransform::Identity;
	AdvancedScene->AddComponent(PreviewMeshComp, Transform);
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->SetPreviewMesh(NewMesh, PreviewMeshComp);
	}
	
}

void SEditorViewport_SimpleSocket::UpdatePreviewSocketMeshes()
{
	if (PreviewMesh)
	{
		const int32 SocketedComponentCount = SocketPreviewMeshComponents.Num();
		const int32 SocketCount = PreviewMesh->Sockets.Num();

		const int32 IterationCount = FMath::Max(SocketedComponentCount, SocketCount);
		for (int32 i = 0; i < IterationCount; ++i)
		{
			if (i >= SocketCount)
			{
				// Handle removing an old component
				UStaticMeshComponent* SocketPreviewMeshComponent = SocketPreviewMeshComponents[i];
				AdvancedScene->RemoveComponent(SocketPreviewMeshComponent);
				SocketPreviewMeshComponents.RemoveAt(i, SocketedComponentCount - i);
				break;
			}
			else if (UStaticMeshSocket* Socket = PreviewMesh->Sockets[i])
			{
				UStaticMeshComponent* SocketPreviewMeshComponent = NULL;

				// Handle adding a new component
				if (i >= SocketedComponentCount)
				{
					SocketPreviewMeshComponent = NewObject<UStaticMeshComponent>();
					AdvancedScene->AddComponent(SocketPreviewMeshComponent, FTransform::Identity);
					SocketPreviewMeshComponents.Add(SocketPreviewMeshComponent);
					SocketPreviewMeshComponent->AttachToComponent(PreviewMeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket->SocketName);
				}
				else
				{
					SocketPreviewMeshComponent = SocketPreviewMeshComponents[i];

					// In case of a socket rename, ensure our preview component is still snapping to the proper socket
					if (!SocketPreviewMeshComponent->GetAttachSocketName().IsEqual(Socket->SocketName))
					{
						SocketPreviewMeshComponent->AttachToComponent(PreviewMeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket->SocketName);
					}

					// Force component to world update to take into account the new socket position.
					SocketPreviewMeshComponent->UpdateComponentToWorld();
				}

				SocketPreviewMeshComponent->SetStaticMesh(Socket->PreviewStaticMesh);
			}
		}
	

	}
}


void SEditorViewport_SimpleSocket::UpdateSelectedSocket(UStaticMeshSocket* InSelectedSocket)
{
	EditorViewportClient.Get()->OnSocketSelectionChanged(InSelectedSocket);
}

TSharedRef<FEditorViewportClient> SEditorViewport_SimpleSocket::MakeEditorViewportClient()
{
	

	OnFocusViewportToSelection();




	EditorViewportClient = MakeShareable(new FEditorViewportClient_SimpleSocket(HUD, SharedThis(this), AdvancedScene.ToSharedRef(), PreviewMesh, PreviewMeshComp));

	return EditorViewportClient.ToSharedRef();

}

TSharedPtr<SWidget> SEditorViewport_SimpleSocket::MakeViewportToolbar()
{
	return SNew(SCommonEditorViewportToolbarBase, SharedThis(this));
}

