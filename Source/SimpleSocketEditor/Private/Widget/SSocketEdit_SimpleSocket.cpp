#include "Widget/SSocketEdit_SimpleSocket.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshEditor/Public/IStaticMeshEditor.h"
#include "Engine/StaticMeshSocket.h"
#include "EngineAnalytics.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widget/SSHud_SimpleSocket.h"
#include "SimpleSocketEditorModule.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "SimpleSocketEditorCommands.h"




#define LOCTEXT_NAMESPACE "SSCSSocketManagerEditor"


struct SocketListItem_SimpleSocketEdit
{
public:
	SocketListItem_SimpleSocketEdit(UStaticMeshSocket* InSocket)
		: Socket(InSocket)
	{
	}

	/** The static mesh socket this represents */
	UStaticMeshSocket* Socket;

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE(FOnRenameRequested);
	FOnRenameRequested OnRenameRequested;
};


//****************************************************************************

class SSocketDisplayItem_SimpleSocket : public STableRow< TSharedPtr<FString> >
{
public:

	SLATE_BEGIN_ARGS(SSocketDisplayItem_SimpleSocket)
	{}

	/** The socket this item displays. */
	SLATE_ARGUMENT(TWeakPtr< SocketListItem_SimpleSocketEdit >, SocketItem)

		/** Pointer back to the socket manager */
		SLATE_ARGUMENT(TWeakPtr< SSocketEdit >, SocketManagerPtr)
		SLATE_END_ARGS()

		/**
		 * Construct the widget
		 *
		 * @param InArgs   A declaration from which to construct the widget
		 */
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SocketItem = InArgs._SocketItem;
		SocketManagerPtr = InArgs._SocketManagerPtr;

		TSharedPtr< SInlineEditableTextBlock > InlineWidget;

		this->ChildSlot
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(InlineWidget, SInlineEditableTextBlock)
				.Text(this, &SSocketDisplayItem_SimpleSocket::GetSocketName)
			.OnVerifyTextChanged(this, &SSocketDisplayItem_SimpleSocket::OnVerifySocketNameChanged)
			.OnTextCommitted(this, &SSocketDisplayItem_SimpleSocket::OnCommitSocketName)
			.IsSelected(this, &STableRow< TSharedPtr<FString> >::IsSelectedExclusively)
			];

		TSharedPtr<SocketListItem_SimpleSocketEdit> SocketItemPinned = SocketItem.Pin();
		if (SocketItemPinned.IsValid())
		{
			SocketItemPinned->OnRenameRequested.BindSP(InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
		}

		STableRow< TSharedPtr<FString> >::ConstructInternal(
			STableRow::FArguments()
			.ShowSelection(true),
			InOwnerTableView
		);
	}
private:
	/** Returns the socket name */
	FText GetSocketName() const
	{
		TSharedPtr<SocketListItem_SimpleSocketEdit> SocketItemPinned = SocketItem.Pin();
		return SocketItemPinned.IsValid() ? FText::FromName(SocketItemPinned->Socket->SocketName) : FText();
	}

	bool OnVerifySocketNameChanged(const FText& InNewText, FText& OutErrorMessage)
	{
		bool bVerifyName = true;

		FText NewText = FText::TrimPrecedingAndTrailing(InNewText);
		if (NewText.IsEmpty())
		{
			OutErrorMessage = LOCTEXT("EmptySocketName_Error", "Sockets must have a name!");
			bVerifyName = false;
		}
		else
		{
			TSharedPtr<SocketListItem_SimpleSocketEdit> SocketItemPinned = SocketItem.Pin();
			TSharedPtr<SSocketEdit> SocketManagerPinned = SocketManagerPtr.Pin();

			if (SocketItemPinned.IsValid() && SocketItemPinned->Socket != nullptr && SocketItemPinned->Socket->SocketName.ToString() != NewText.ToString() &&
				SocketManagerPinned.IsValid() && SocketManagerPinned->CheckForDuplicateSocket(NewText.ToString()))
			{
				OutErrorMessage = LOCTEXT("DuplicateSocket_Error", "Socket name in use!");
				bVerifyName = false;
			}
		}

		return bVerifyName;
	}

	void OnCommitSocketName(const FText& InText, ETextCommit::Type CommitInfo)
	{
		FText NewText = FText::TrimPrecedingAndTrailing(InText);

		TSharedPtr<SocketListItem_SimpleSocketEdit> PinnedSocketItem = SocketItem.Pin();
		if (PinnedSocketItem.IsValid())
		{
			UStaticMeshSocket* SelectedSocket = PinnedSocketItem->Socket;
			if (SelectedSocket != NULL)
			{
				FScopedTransaction Transaction(LOCTEXT("SetSocketName", "Set Socket Name"));

				FProperty* ChangedProperty = FindFProperty<FProperty>(UStaticMeshSocket::StaticClass(), "SocketName");

				// Pre edit, calls modify on the object
				SelectedSocket->PreEditChange(ChangedProperty);

				// Edit the property itself
				SelectedSocket->SocketName = FName(*NewText.ToString());

				// Post edit
				FPropertyChangedEvent PropertyChangedEvent(ChangedProperty);
				SelectedSocket->PostEditChangeProperty(PropertyChangedEvent);
			}
		}
	}

private:
	/** The Socket to display. */
	TWeakPtr< SocketListItem_SimpleSocketEdit > SocketItem;

	/** Pointer back to the socket manager */
	TWeakPtr< SSocketEdit > SocketManagerPtr;
};



//*********************************************************************************






void SSocketEdit::Construct(const FArguments& InArgs)
{
	StaticMesh = InArgs._StaticMesh;
	HUD = InArgs._HUD;
	OnSocketSelectionChanged = InArgs._OnSocketSelectionChanged;


	if (!StaticMesh || !HUD.IsValid())
	{
		return;
	}
	
	// Register a post undo function which keeps the socket manager list view consistent with the static mesh
	//StaticMeshEditorPinned->RegisterOnPostUndo(IStaticMeshEditor::FOnPostUndo::CreateSP(this, &SSocketEdit::PostUndo));


	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.bAllowSearch = false;
	Args.bShowOptions = false;
	Args.NotifyHook = this;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	SocketDetailsView = PropertyModule.CreateDetailView(Args);

	WorldSpaceRotation = FVector::ZeroVector;
	//WorldSpaceRotation = FVector(0.0f, -45.0f, 0.0f);

	this->ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)

		+ SSplitter::Slot()
		.Value(.3f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
		.ForegroundColor(FLinearColor::White)
		.Text(LOCTEXT("CreateSocket", "Create Socket"))
		.OnClicked(this, &SSocketEdit::CreateSocket_Execute)
		.HAlign(HAlign_Center)
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(SocketListView, SListView<TSharedPtr< SocketListItem_SimpleSocketEdit > >)

			.SelectionMode(ESelectionMode::Single)

		.ListItemsSource(&SocketList)

		// Generates the actual widget for a tree item
		.OnGenerateRow(this, &SSocketEdit::MakeWidgetFromOption)

		// Find out when the user selects something in the tree
		.OnSelectionChanged(this, &SSocketEdit::SocketSelectionChanged_Execute)

		// Allow for some spacing between items with a larger item height.
		.ItemHeight(20.0f)

		.OnContextMenuOpening(this, &SSocketEdit::OnContextMenuOpening)
		.OnItemScrolledIntoView(this, &SSocketEdit::OnItemScrolledIntoView)
		.OnKeyDownHandler(this, &SSocketEdit::OnKeyDown)

		.HeaderRow
		(
			SNew(SHeaderRow)
			.Visibility(EVisibility::Collapsed)
			+ SHeaderRow::Column(TEXT("Socket"))
		)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(this, &SSocketEdit::GetSocketHeaderText)
		]
		]
		]

	+ SSplitter::Slot()
		.Value(.7f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(this, &SSocketEdit::GetSelectSocketMessageVisibility)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoSocketSelected", "Select a Socket"))
		]
		]

	+ SOverlay::Slot()
		[
			SocketDetailsView.ToSharedRef()
		]
		]
		]
		];

	RefreshSocketList();

	AddPropertyChangeListenerToSockets();

	BindCommands();
}


SSocketEdit::~SSocketEdit()
{

}

bool SSocketEdit::CanDeleteSelected() const
{
	return GetSelectedSocket() != nullptr;
}

void SSocketEdit::DuplicatedSelected()
{
	DuplicateSelectedSocket();
}

bool SSocketEdit::CanDuplicatedSelected() const
{
	return GetSelectedSocket() != nullptr;
}

void SSocketEdit::RenameSelected()
{
	RequestRenameSelectedSocket();
}

bool SSocketEdit::CanRenameSelected() const
{
	return GetSelectedSocket() != nullptr;
}

FReply SSocketEdit::OnKeyDown(const FGeometry& geo, const FKeyEvent& keyEvent)
{
	if (keyEvent.GetKey() == EKeys::Delete)
	{
		DeleteSelectedSocket();
	}
	else if(keyEvent.GetKey() == EKeys::F2)
	{
		RenameSelected();
	}
	else if (keyEvent.GetKey() == EKeys::W && keyEvent.IsControlDown())
	{
		DuplicatedSelected();
	}

	return FReply::Handled();
}
UStaticMeshSocket* SSocketEdit::GetSelectedSocket() const
{
	if (SocketListView->GetSelectedItems().Num())
	{
		return SocketListView->GetSelectedItems()[0]->Socket;
	}

	return nullptr;
}

void SSocketEdit::SetSelectedSocket(UStaticMeshSocket* InSelectedSocket)
{
	if (InSelectedSocket)
	{
		for (int32 i = 0; i < SocketList.Num(); i++)
		{
			if (SocketList[i]->Socket == InSelectedSocket)
			{
				SocketListView->SetSelection(SocketList[i]);

				SocketListView->RequestListRefresh();

				SocketSelectionChanged(InSelectedSocket);

				break;
			}
		}
	}
	else
	{
		SocketListView->ClearSelection();

		SocketListView->RequestListRefresh();

		SocketSelectionChanged(NULL);
	}
}

void SSocketEdit::DeleteSelectedSocket()
{
	if (SocketListView->GetSelectedItems().Num())
	{
		const FScopedTransaction Transaction(LOCTEXT("DeleteSocket", "Delete Socket"));
	

			UStaticMesh* CurrentStaticMesh = StaticMesh;
			CurrentStaticMesh->PreEditChange(NULL);
			UStaticMeshSocket* SelectedSocket = SocketListView->GetSelectedItems()[0]->Socket;
			SelectedSocket->OnPropertyChanged().RemoveAll(this);
			CurrentStaticMesh->Sockets.Remove(SelectedSocket);
			CurrentStaticMesh->PostEditChange();

			RefreshSocketList();
		
		

	}
}

void SSocketEdit::DuplicateSelectedSocket()
{
	//TSharedPtr<SSHud_SimpleSocket> hud = HUD.Pin();

	UStaticMeshSocket* SelectedSocket = GetSelectedSocket();

	if (StaticMesh && SelectedSocket)
	{
		const FScopedTransaction Transaction(LOCTEXT("SocketManager_DuplicateSocket", "Duplicate Socket"));

		UStaticMesh* CurrentStaticMesh = StaticMesh;

		UStaticMeshSocket* NewSocket = DuplicateObject(SelectedSocket, CurrentStaticMesh);

		// Create a unique name for this socket
		NewSocket->SocketName = MakeUniqueObjectName(CurrentStaticMesh, UStaticMeshSocket::StaticClass(), NewSocket->SocketName);

		// Add the new socket to the static mesh
		CurrentStaticMesh->PreEditChange(NULL);
		CurrentStaticMesh->AddSocket(NewSocket);
		CurrentStaticMesh->PostEditChange();
		CurrentStaticMesh->MarkPackageDirty();

		RefreshSocketList();

		// Select the duplicated socket
		SetSelectedSocket(NewSocket);
	}
}

void SSocketEdit::RequestRenameSelectedSocket()
{
	if (SocketListView->GetSelectedItems().Num() == 1)
	{
		TSharedPtr< SocketListItem_SimpleSocketEdit > SocketItem = SocketListView->GetSelectedItems()[0];
		SocketListView->RequestScrollIntoView(SocketItem);
		DeferredRenameRequest = SocketItem;
	}
}

void SSocketEdit::UpdateStaticMesh()
{
	RefreshSocketList();
}

void SSocketEdit::SetNewMesh(UStaticMesh* newMesh)
{
	StaticMesh = newMesh;
	UpdateStaticMesh();
}

bool SSocketEdit::CheckForDuplicateSocket(const FString& InSocketName)
{
	for (int32 i = 0; i < SocketList.Num(); i++)
	{
		if (SocketList[i]->Socket->SocketName.ToString() == InSocketName)
		{
			return true;
		}
	}

	return false;
}

TSharedRef< ITableRow > SSocketEdit::MakeWidgetFromOption(TSharedPtr< struct SocketListItem_SimpleSocketEdit> InItem, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SSocketDisplayItem_SimpleSocket, OwnerTable)
		.SocketItem(InItem)
		.SocketManagerPtr(SharedThis(this));
}

void SSocketEdit::CreateSocket()
{
	TSharedPtr<SSHud_SimpleSocket> hud = HUD.Pin();

	if (StaticMesh)
	{
		UStaticMesh* CurrentStaticMesh = StaticMesh;

		const FScopedTransaction Transaction(LOCTEXT("CreateSocket", "Create Socket"));

		UStaticMeshSocket* NewSocket = NewObject<UStaticMeshSocket>(CurrentStaticMesh);
		check(NewSocket);

		if (FEngineAnalytics::IsAvailable())
		{
			FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.StaticMesh.CreateSocket"));
		}

		FString SocketNameString = TEXT("Socket");
		FName SocketName = FName(*SocketNameString);

		// Make sure the new name is valid
		int32 Index = 0;
		while (CheckForDuplicateSocket(SocketName.ToString()))
		{
			SocketName = FName(*FString::Printf(TEXT("%s%i"), *SocketNameString, Index));
			++Index;
		}


		NewSocket->SocketName = SocketName;
		NewSocket->SetFlags(RF_Transactional);
		NewSocket->OnPropertyChanged().AddSP(this, &SSocketEdit::OnSocketPropertyChanged);

		CurrentStaticMesh->PreEditChange(NULL);
		CurrentStaticMesh->AddSocket(NewSocket);
		CurrentStaticMesh->PostEditChange();
		CurrentStaticMesh->MarkPackageDirty();

		TSharedPtr< SocketListItem_SimpleSocketEdit > SocketItem = MakeShareable(new SocketListItem_SimpleSocketEdit(NewSocket));
		SocketList.Add(SocketItem);
		SocketListView->RequestListRefresh();

		SocketListView->SetSelection(SocketItem);
		RequestRenameSelectedSocket();
	}
}

void SSocketEdit::RefreshSocketList()
{
	auto HUDPin = HUD.Pin();
	if (StaticMesh && HUD.IsValid())
	{
		// Only rebuild the socket list if it differs from the static meshes socket list
		// This is done so that an undo on a socket property doesn't cause the selected
		// socket to be de-selected, thus hiding the socket properties on the detail view.
		// NB: Also force a rebuild if the underlying StaticMesh has been changed.
		
		SocketList.Empty();
		for (int32 i = 0; i < StaticMesh->Sockets.Num(); i++)
		{
			UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
			SocketList.Add(MakeShareable(new SocketListItem_SimpleSocketEdit(Socket)));
		}

		SocketListView->RequestListRefresh();
		

		// Set the socket on the detail view to keep it in sync with the sockets properties
		if (SocketListView->GetSelectedItems().Num())
		{
			TArray< UObject* > ObjectList;
			ObjectList.Add(SocketListView->GetSelectedItems()[0]->Socket);
			SocketDetailsView->SetObjects(ObjectList, true);
		}

		//HUD.Pin()->GetViewPort()->
		//StaticMeshEditorPinned->RefreshViewport();
		//HUDPin->GetViewPort()->Invalidate();
	}
	else
	{
		SocketList.Empty();
		SocketListView->ClearSelection();
		SocketListView->RequestListRefresh();
	}
}

EVisibility SSocketEdit::GetSelectSocketMessageVisibility() const
{
	return SocketListView->GetSelectedItems().Num() > 0 ? EVisibility::Hidden : EVisibility::Visible;
}

void SSocketEdit::SocketSelectionChanged(UStaticMeshSocket* InSocket)
{
	TArray<UObject*> SelectedObject;

	if (InSocket)
	{
		SelectedObject.Add(InSocket);
	}

	SocketDetailsView->SetObjects(SelectedObject);

	// Notify listeners
	OnSocketSelectionChanged.ExecuteIfBound();
}

void SSocketEdit::SocketSelectionChanged_Execute(TSharedPtr<SocketListItem_SimpleSocketEdit> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		SocketSelectionChanged(InItem->Socket);
	}
	else
	{
		SocketSelectionChanged(NULL);
	}
}

FReply SSocketEdit::CreateSocket_Execute()
{
	CreateSocket();

	return FReply::Handled();
}

FText SSocketEdit::GetSocketHeaderText() const
{
	UStaticMesh* CurrentStaticMesh = nullptr;
	

	if (StaticMesh)
	{
		CurrentStaticMesh = StaticMesh;
	}
	return FText::Format(LOCTEXT("SocketHeader_TotalFmt", "{0} sockets"), FText::AsNumber((CurrentStaticMesh != nullptr) ? CurrentStaticMesh->Sockets.Num() : 0));
}

void SSocketEdit::SocketName_TextChanged(const FText& InText)
{
	CheckForDuplicateSocket(InText.ToString());
}

TSharedPtr<SWidget> SSocketEdit::OnContextMenuOpening()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	TSharedPtr<SSHud_SimpleSocket> hud = HUD.Pin();

	if (!hud.IsValid())
	{
		return TSharedPtr<SWidget>();
	}

	FSimpleSocketEditorModule& mod = FModuleManager::LoadModuleChecked<FSimpleSocketEditorModule>("SimpleSocketEditor");


	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SSocketEdit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	TArray< TSharedPtr< SocketListItem_SimpleSocketEdit > > SelectedList = SocketListView->GetSelectedItems();
	if (SelectedList.Num())
	{
		if (PropertyThatChanged->GetName() == TEXT("Pitch") || PropertyThatChanged->GetName() == TEXT("Yaw") || PropertyThatChanged->GetName() == TEXT("Roll"))
		{
			const UStaticMeshSocket* Socket = SelectedList[0]->Socket;
			WorldSpaceRotation.Set(Socket->RelativeRotation.Pitch, Socket->RelativeRotation.Yaw, Socket->RelativeRotation.Roll);
		}
	}
}

void SSocketEdit::PostUndo()
{
	RefreshSocketList();
}

void SSocketEdit::OnItemScrolledIntoView(TSharedPtr<SocketListItem_SimpleSocketEdit> InItem, const TSharedPtr<ITableRow>& InWidget)
{
	TSharedPtr<SocketListItem_SimpleSocketEdit> DeferredRenameRequestPinned = DeferredRenameRequest.Pin();
	if (DeferredRenameRequestPinned.IsValid())
	{
		DeferredRenameRequestPinned->OnRenameRequested.ExecuteIfBound();
		DeferredRenameRequest.Reset();
	}
}

void SSocketEdit::DeleteSelected()
{
	if (GetSelectedSocket())
	{
		DeleteSelectedSocket();
	}
}



void SSocketEdit::BindCommands()
{
	const FSimpleSocketEditorCommands& Commands = FSimpleSocketEditorCommands::Get();

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SSocketEdit::DeleteSelected),
		FCanExecuteAction::CreateSP(this, &SSocketEdit::CanDeleteSelected));

	CommandList->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &SSocketEdit::DuplicatedSelected),
		FCanExecuteAction::CreateSP(this, &SSocketEdit::CanDuplicatedSelected));

	CommandList->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SSocketEdit::RenameSelected),
		FCanExecuteAction::CreateSP(this, &SSocketEdit::CanRenameSelected));

}

void SSocketEdit::AddPropertyChangeListenerToSockets()
{
	if (StaticMesh)
	{
		UStaticMesh* CurrentStaticMesh = StaticMesh;
		for (int32 i = 0; i < CurrentStaticMesh->Sockets.Num(); ++i)
		{
			CurrentStaticMesh->Sockets[i]->OnPropertyChanged().AddSP(this, &SSocketEdit::OnSocketPropertyChanged);
		}
	}
}

void SSocketEdit::RemovePropertyChangeListenerFromSockets()
{
	if (StaticMesh)
	{
		UStaticMesh* CurrentStaticMesh = StaticMesh;
		if (CurrentStaticMesh)
		{
			for (int32 i = 0; i < CurrentStaticMesh->Sockets.Num(); ++i)
			{
				CurrentStaticMesh->Sockets[i]->OnPropertyChanged().RemoveAll(this);
			}
		}
	}
}

void SSocketEdit::OnSocketPropertyChanged(const UStaticMeshSocket* Socket, const FProperty* ChangedProperty)
{
	static FName RelativeRotationName(TEXT("RelativeRotation"));
	static FName RelativeLocationName(TEXT("RelativeLocation"));

	check(Socket != nullptr);

	FName ChangedPropertyName = ChangedProperty->GetFName();

	if (ChangedPropertyName == RelativeRotationName)
	{
		const UStaticMeshSocket* SelectedSocket = GetSelectedSocket();

		if (Socket == SelectedSocket)
		{
			WorldSpaceRotation.Set(Socket->RelativeRotation.Pitch, Socket->RelativeRotation.Yaw, Socket->RelativeRotation.Roll);
		}
	}

	//TSharedPtr<IStaticMeshEditor> StaticMeshEditorPinned = StaticMeshEditorPtr.Pin();
	if (!StaticMesh)
	{
		return;
	}

	if (ChangedPropertyName == RelativeRotationName || ChangedPropertyName == RelativeLocationName)
	{
		// If socket location or rotation is changed, update the position of any actors attached to it in instances of this mesh
		UStaticMesh* CurrentStaticMesh = StaticMesh;
		if (CurrentStaticMesh != nullptr)
		{
			bool bUpdatedChild = false;

			for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
			{
				if (It->GetStaticMesh() == CurrentStaticMesh)
				{
					const AActor* Actor = It->GetOwner();
					if (Actor != nullptr)
					{
						const USceneComponent* Root = Actor->GetRootComponent();
						if (Root != nullptr)
						{
							for (USceneComponent* Child : Root->GetAttachChildren())
							{
								if (Child != nullptr && Child->GetAttachSocketName() == Socket->SocketName)
								{
									Child->UpdateComponentToWorld();
									bUpdatedChild = true;
								}
							}
						}
					}
				}
			}

			if (bUpdatedChild)
			{
				GUnrealEd->RedrawLevelEditingViewports();
			}
		}
	}
}




#undef LOCTEXT_NAMESPACE