// Copyright 2018 Jack Knobel. All Rights Reserved.

#include "AsyncLoadingLibrary.h"

// UE Includes
#include "Engine/AssetManager.h"
#include "LatentActions.h"
#include "Delegates/Delegate.h"

DEFINE_LOG_CATEGORY_STATIC(AsyncLoadLibrary, Log, All)

// -----------------------------------------------------------------------------------------
void FAsyncLoadBundle::TriggerLoad(TAsyncLoadPriority loadPriority /* = 50 */, const FString& debugName /* = TEXT("LoadAsyncBundle") */)
{
	TArray<FSoftObjectPath> objectsRequiringLoad;
	objectsRequiringLoad.Reserve(ObjectBundle.Num());

	for (const FBundleAsset& asset : ObjectBundle)
	{
		if (asset.GetObject() == nullptr)
		{
			objectsRequiringLoad.Emplace(asset.GetPath());
		}
	}
	objectsRequiringLoad.Shrink();

	if (objectsRequiringLoad.Num() > 0)
	{
		UAssetManager& assetManager = UAssetManager::Get();
		assetManager.LoadAssetList(objectsRequiringLoad, CallbackDelegate, loadPriority, debugName);
	}
	else
	{
		CallbackDelegate.ExecuteIfBound();
	}
}

// =========================================================================================
/* Latent Action, modified version of the one defined in KismetSystemLibrary.cpp */
struct FLoadAssetActionBase : public FPendingLatentAction
{
public:
	TArray<FSoftObjectPath> SoftObjectPaths;

	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> Handle;

	UAsyncLoadingLibrary::FOnAssetsLoaded OnLoadedCallback;

	FWeakObjectPtr CallbackTarget;
	FName ExecutionFunction;
	int32 OutputLink;

	// -----------------------------------------------------------------------------------------
	FLoadAssetActionBase(const TArray<FSoftObjectPath>& softObjectPaths, const UAsyncLoadingLibrary::FOnAssetsLoaded& callBackDelegate, const FLatentActionInfo& latentInfo)
		: SoftObjectPaths(softObjectPaths)
		, ExecutionFunction(latentInfo.ExecutionFunction)
		, OutputLink(latentInfo.Linkage)
		, OnLoadedCallback(callBackDelegate)
		, CallbackTarget(latentInfo.CallbackTarget)
	{
		Handle = StreamableManager.RequestAsyncLoad(SoftObjectPaths);
	}

	// -----------------------------------------------------------------------------------------
	FLoadAssetActionBase(const TSet<FSoftObjectPath>& softObjectPaths, const UAsyncLoadingLibrary::FOnAssetsLoaded& callBackDelegate, const FLatentActionInfo& latentInfo)
		: FLoadAssetActionBase(softObjectPaths.Array(), callBackDelegate, latentInfo) {}

	// -----------------------------------------------------------------------------------------
	virtual ~FLoadAssetActionBase()
	{
		if (Handle.IsValid())
		{
			Handle->ReleaseHandle();
		}
	}

	// -----------------------------------------------------------------------------------------
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		const bool bLoaded = !Handle.IsValid() || Handle->HasLoadCompleted() || Handle->WasCanceled();
		if (bLoaded)
		{
			OnLoaded();
		}
		Response.FinishAndTriggerIf(bLoaded, ExecutionFunction, OutputLink, CallbackTarget);
	}

	// -----------------------------------------------------------------------------------------
	virtual void OnLoaded()
	{
		/* Pointless resolving all of their names if there is no delegate bound */
		if (OnLoadedCallback.IsBound())
		{
			TArray<UObject*> loadedObjects;
			loadedObjects.Reserve(SoftObjectPaths.Num());
			for (const auto& softObjectPath : SoftObjectPaths)
			{
				if (UObject* resolvedObject = softObjectPath.ResolveObject())
				{
					loadedObjects.Add(resolvedObject);
				}
				else
				{
					UE_LOG(AsyncLoadLibrary, Warning, TEXT("Failed to load/resolve Object for path: ​%s"), *softObjectPath.GetAssetPathString());
				}
			}
			OnLoadedCallback.Execute(loadedObjects);
		}
	}


#if WITH_EDITOR
	// -----------------------------------------------------------------------------------------
	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("FLoadAssetActionBase - Loading %d Assets"), SoftObjectPaths.Num());
	}
#endif
};

// -----------------------------------------------------------------------------------------
void UAsyncLoadingLibrary::AsyncLoadAnyAssets(UObject* worldContextObject, const TSet<FSoftObjectPath>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (LatentManager.FindExistingAction<FLoadAssetActionBase>(latentAction.CallbackTarget, latentAction.UUID) == nullptr)
		{
			FLoadAssetActionBase* NewAction = new FLoadAssetActionBase(assets, onAssetsLoaded, latentAction);
			LatentManager.AddNewAction(latentAction.CallbackTarget, latentAction.UUID, NewAction);
		}
	}
}

// -----------------------------------------------------------------------------------------
void UAsyncLoadingLibrary::AsyncLoadAssets(UObject* worldContextObject, const TSet<TSoftObjectPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (LatentManager.FindExistingAction<FLoadAssetActionBase>(latentAction.CallbackTarget, latentAction.UUID) == nullptr)
		{
			TArray<FSoftObjectPath> softObjectPaths;
			UAsyncLoadingLibrary::ConvertSoftPtrSetToSoftObjectPathArray(assets, softObjectPaths);

			FLoadAssetActionBase* NewAction = new FLoadAssetActionBase(softObjectPaths, onAssetsLoaded, latentAction);
			LatentManager.AddNewAction(latentAction.CallbackTarget, latentAction.UUID, NewAction);
		}
	}
}

// -----------------------------------------------------------------------------------------
void UAsyncLoadingLibrary::AsyncLoadClasses(UObject* worldContextObject, const TSet<TSoftClassPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (LatentManager.FindExistingAction<FLoadAssetActionBase>(latentAction.CallbackTarget, latentAction.UUID) == nullptr)
		{
			TArray<FSoftObjectPath> softObjectPaths;
			UAsyncLoadingLibrary::ConvertSoftPtrSetToSoftObjectPathArray(assets, softObjectPaths);

			FLoadAssetActionBase* NewAction = new FLoadAssetActionBase(softObjectPaths, onAssetsLoaded, latentAction);
			LatentManager.AddNewAction(latentAction.CallbackTarget, latentAction.UUID, NewAction);
		}
	}
}
