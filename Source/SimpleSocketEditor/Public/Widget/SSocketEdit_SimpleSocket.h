// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/NotifyHook.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SSpinBox.h"
#include "IDetailsView.h"
#include "ISocketManager.h"

class IStaticMeshEditor;
class UStaticMesh;
class UStaticMeshSocket;
struct FPropertyChangedEvent;
struct SocketListItem_SimpleSocketEdit;
class SSHud_SimpleSocket;

class SSocketEdit : public ISocketManager, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SSocketEdit) :
		_StaticMesh(),_HUD()
		{}
		SLATE_ARGUMENT(UStaticMesh*, StaticMesh)
		SLATE_ARGUMENT(TSharedPtr< SSHud_SimpleSocket >, HUD)
		SLATE_EVENT(FSimpleDelegate, OnSocketSelectionChanged)
	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs);

	virtual ~SSocketEdit();

	// ISocketManager interface
	virtual UStaticMeshSocket* GetSelectedSocket() const override;
	virtual void SetSelectedSocket(UStaticMeshSocket* InSelectedSocket) override;
	virtual void DeleteSelectedSocket() override;
	virtual void DuplicateSelectedSocket() override;
	virtual void RequestRenameSelectedSocket() override;
	virtual void UpdateStaticMesh() override;
	// End of ISocketManager



	void SetNewMesh(UStaticMesh* newMesh);

		/**
	 *	Checks for a duplicate socket using the name for comparison.
	 *
	 *	@param InSocketName			The name to compare.
	 *
	 *	@return						TRUE if another socket already exists with that name.
	 */
	bool CheckForDuplicateSocket(const FString& InSocketName);

private:
	/** Creates a widget from the list item. */
	TSharedRef< ITableRow > MakeWidgetFromOption(TSharedPtr< struct SocketListItem_SimpleSocketEdit> InItem, const TSharedRef< STableViewBase >& OwnerTable);

	/**	Creates a socket with a specified name. */
	void CreateSocket();

	/** Refreshes the socket list. */
	void RefreshSocketList();

	/** Gets the visibility of the select a socket message */
	EVisibility GetSelectSocketMessageVisibility() const;

	/**
	 *	Updates the details to the selected socket.
	 *
	 *	@param InSocket				The newly selected socket.
	 */
	void SocketSelectionChanged(UStaticMeshSocket* InSocket);

	/** Callback for the list view when an item is selected. */
	void SocketSelectionChanged_Execute(TSharedPtr<SocketListItem_SimpleSocketEdit> InItem, ESelectInfo::Type SelectInfo);

	/** Callback for the Create Socket button. */
	FReply CreateSocket_Execute();

	FText GetSocketHeaderText() const;

	/** Callback for when the socket name textbox is changed, verifies the name is not a duplicate. */
	void SocketName_TextChanged(const FText& InText);

	/** Callback to retrieve the context menu for the list view */
	TSharedPtr<SWidget> OnContextMenuOpening();

	/** FNotifyHook interface */
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	/** Post undo */
	void PostUndo();

	/** Callback when an item is scrolled into view, handling calls to rename items */
	void OnItemScrolledIntoView(TSharedPtr<SocketListItem_SimpleSocketEdit> InItem, const TSharedPtr<ITableRow>& InWidget);



	void BindCommands();

private:
	void DeleteSelected();
	bool CanDeleteSelected() const;
	void DuplicatedSelected();
	bool CanDuplicatedSelected() const;
	void RenameSelected();
	bool CanRenameSelected() const;


	FReply OnKeyDown(const FGeometry& geo, const FKeyEvent& keyEvent);

	/** Add a property change listener to each socket. */
	void AddPropertyChangeListenerToSockets();

	/** Remove the property change listener from the sockets. */
	void RemovePropertyChangeListenerFromSockets();

	/** Called when a socket property has changed. */
	void OnSocketPropertyChanged(const UStaticMeshSocket* Socket, const FProperty* ChangedProperty);

	/** Called when socket selection changes */
	FSimpleDelegate OnSocketSelectionChanged;

	/** Pointer back to the static mesh editor, used for */
	TWeakPtr< SSHud_SimpleSocket > HUD;


	/** Details panel for the selected socket. */
	TSharedPtr<class IDetailsView> SocketDetailsView;

	/** List of sockets for for the associated static mesh or anim set. */
	TArray< TSharedPtr<SocketListItem_SimpleSocketEdit> > SocketList;

	/** Listview for displaying the sockets. */
	TSharedPtr< SListView<TSharedPtr< SocketListItem_SimpleSocketEdit > > > SocketListView;

	/** Helper variable for rotating in world space. */
	FVector WorldSpaceRotation;



	/** The static mesh being edited. */
	 UStaticMesh* StaticMesh;

	/** Widgets for the World Space Rotation */
	TSharedPtr< SSpinBox<float> > PitchRotation;
	TSharedPtr< SSpinBox<float> > YawRotation;
	TSharedPtr< SSpinBox<float> > RollRotation;

	/** Points to an item that is being requested to be renamed */
	TWeakPtr<SocketListItem_SimpleSocketEdit> DeferredRenameRequest;

	TSharedPtr<class FUICommandList> CommandList;
};
