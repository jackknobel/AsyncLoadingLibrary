// Copyright 2018 Jack Knobel. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/StreamableManager.h"
#include "AsyncLoadingLibrary.generated.h"

struct FBundleAsset
{
private:
	FSoftObjectPath ObjectPath;

public:
	template<typename TSoftAsset>
	FBundleAsset(const TSoftAsset& asset)
		: ObjectPath(asset.ToSoftObjectPath())
	{}

	FBundleAsset(const FSoftObjectPath& asset)
		: ObjectPath(asset)
	{}

	inline const FSoftObjectPath& GetPath() const { return ObjectPath; }

	inline const UObject* GetObject() const { return ObjectPath.ResolveObject(); }
};

using FAssetBundle = TArray<FBundleAsset>;

struct FAsyncLoadBundle
{
	// Objects we would like to load
	FAssetBundle ObjectBundle;

	// A callback function for the loading of the bundle
	FStreamableDelegate CallbackDelegate;

	template<typename TAsset>
	FAsyncLoadBundle(const FAssetBundle& objectsToLoad, const FStreamableDelegate& loadCompleteCallback = FStreamableDelegate())
		: ObjectBundle(objectsToLoad), CallbackDelegate(loadCompleteCallback)
	{}

	template<typename TAsset>
	inline bool AddAssetToLoad(const TAsset& asset)
	{
		ObjectBundle.Emplace(asset.ToSoftObjectPath());
	}

	template<typename TAsset>
	inline bool AddAssetsToLoad(const FAssetBundle& assets)
	{
		ObjectBundle.Append(assets);
	}

	void TriggerLoad(TAsyncLoadPriority loadPriority = 50, const FString& debugName = TEXT("LoadAsyncBundle"));
};

/**
 *
 */
UCLASS()
class UAsyncLoadingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAssetsLoaded, const TArray<UObject*>&, LoadedAssets);

	/* Asynchronously load a Set of Soft Object Paths with the option to bind to a delegate to be notified when the loading is complete.
	 * Delegate returns an Array of Loaded UObject's however you are safe to use the original Soft Objects as they will be loaded
	 */
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "latentAction", WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadAnyAssets(UObject* worldContextObject, const TSet<FSoftObjectPath>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadAnyAssets_Event(UObject* worldContextObject, const TSet<FSoftObjectPath>& assets, const FOnAssetsLoaded& onAssetsLoaded)
	{
		FLatentActionInfo latentAction;
		latentAction.CallbackTarget = worldContextObject;
		AsyncLoadAnyAssets(worldContextObject, assets, onAssetsLoaded, latentAction);
	}

	/* Asynchronously load a Set of Soft Object Ptrs with the option to bind to a delegate to be notified when the loading is complete
	 * Delegate returns an Array of Loaded UObject's however you are safe to use the original Soft Objects as they will be loaded
	 */
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "latentAction", WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadAssets(UObject* worldContextObject, const TSet<TSoftObjectPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadAssets_Event(UObject* worldContextObject, const TSet<TSoftObjectPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded)
	{
		FLatentActionInfo latentAction;
		latentAction.CallbackTarget = worldContextObject;
		AsyncLoadAssets(worldContextObject, assets, onAssetsLoaded, latentAction);
	}

	/* Asynchronously load a Set of Soft Class Ptrs with the option to bind to a delegate to be notified when the loading is complete
	 * Delegate returns an Array of Loaded UObject's however you are safe to use the original Soft Objects as they will be loaded
	 */
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "latentAction", WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadClasses(UObject* worldContextObject, const TSet<TSoftClassPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded, FLatentActionInfo latentAction);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "onAssetsLoaded"), Category = AsyncLoadingLibrary)
	static void AsyncLoadClasses_Event(UObject* worldContextObject, const TSet<TSoftClassPtr<UObject>>& assets, const FOnAssetsLoaded& onAssetsLoaded)
	{
		FLatentActionInfo latentAction;
		latentAction.CallbackTarget = worldContextObject;
		AsyncLoadClasses(worldContextObject, assets, onAssetsLoaded, latentAction);
	}

	//////////////////////////////////////////////////////////////////////////
	// Helper Functions
	//////////////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintPure, Category = AsyncLoadingLibrary)
	static inline FSoftObjectPath SoftObjectPtrToSoftObjectPath(const TSoftObjectPtr<UObject>& asset) { return asset.ToSoftObjectPath(); }

	UFUNCTION(BlueprintPure, Category = AsyncLoadingLibrary)
	static inline FSoftObjectPath SoftClassPtrToSoftObjectPath(const TSoftClassPtr<UObject>& asset) { return asset.ToSoftObjectPath(); }

	template<typename TSoftAsset>
	static void ConvertSoftPtrSetToSoftObjectPathArray(const TSet<TSoftAsset>& assets, TArray<FSoftObjectPath>& outSoftObjectPaths)
	{
		outSoftObjectPaths.Reserve(outSoftObjectPaths.Num() + assets.Num());
		for (const auto& asset : assets)
		{
			outSoftObjectPaths.Emplace(asset.ToSoftObjectPath());
		}
	}
};
