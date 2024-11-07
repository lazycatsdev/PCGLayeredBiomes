/*
 * Copyright (c) 2024 LazyCatsDev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#pragma once

#include "CoreMinimal.h"
#include "BiomesSpawnManager.h"
#include "Data/PCGSpatialData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Metadata/PCGAttributePropertySelector.h"
#include "Metadata/Accessors/IPCGAttributeAccessor.h"
#include "Metadata/Accessors/PCGAttributeAccessorKeys.h"
#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"
#include "BiomesPCGUtils.generated.h"

struct FBiomesPersistentInstancesData;

/**
 * Handle which identify mesh instance across all PCG Components in the world.
 * Can be stored in any place and used after game restart.
 * Invalidates if the world rebuilt with different seed or any changes to config or graphs were made.
 */
USTRUCT(BlueprintType, Category=Biomes)
struct FBiomesInstanceHandle
{
	GENERATED_BODY()

	FBiomesInstanceHandle() = default;
	
	explicit operator bool () const { return IsValid(); }

	bool IsValid() const { return InstanceId != INDEX_NONE && ComponentName != NAME_None; }
	
public:
	/**
	 * ID of a group.
	 * Zero value is invalid.
	 * Negative values are indices (-1) of non-partitioned PCG components.
	 * Positive values are indices (+1) of APCGPartitionActor.
	 */
	UPROPERTY()
	int16 GroupId = 0;

	/**
	 * ID of the mesh.
	 */
	UPROPERTY()
	FName ComponentName = NAME_None;
	
	/**
	 * ID of the instance.
	 * Actually it's original index of instance in ISM. 
	 */
	UPROPERTY()
	int32 InstanceId = INDEX_NONE;
};

/**
 * 
 */
UCLASS()
class PCGLAYEREDBIOMES_API UBiomesPCGUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category=Biomes)
	static UBiomesInstanceUserData* ExtractUserData(
		const UInstancedStaticMeshComponent* Component,
		const int32 InstanceId);

	UFUNCTION(BlueprintCallable, Category=Biomes)
	static bool ExtractSpawnInfo(
		const UInstancedStaticMeshComponent* Component,
		const int32 InstanceId, FPCGSpawnInfo& Result);

	UFUNCTION(BlueprintCallable, Category=Biomes, meta=(ExpandBoolAsExecs = "ReturnValue"))
	static bool RemoveInstance(UInstancedStaticMeshComponent* Component, const int32 InstanceId,
		FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform);

	UFUNCTION(BlueprintCallable, Category=Biomes)
	static bool RestoreInstance(const FBiomesInstanceHandle& InstanceHandle);

	UFUNCTION(BlueprintCallable, Category=Biomes, meta=(ExpandBoolAsExecs = "ReturnValue"))
	static bool GetTransformByHandle(const FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform);

	UFUNCTION(BlueprintCallable, Category=Biomes)
	static UBiomesInstanceUserData* GetUserDataByHandle(const FBiomesInstanceHandle& InstanceHandle);

	/**
	 * Return structure which contains all information about removed instances in the world.
	 * Can be stored anywhere and restored later with SetPersistentData
	 */
	UFUNCTION(BlueprintCallable, Category=Biomes)
	static FBiomesPersistentInstancesData GetPersistentData();
	
	/**
	 * Restore information about all removed instances in the world. 
	 * @param Data Which was returned by GetPersistentData.
	 */
	UFUNCTION(BlueprintCallable, Category=Biomes)
	static void SetPersistentData(const FBiomesPersistentInstancesData& Data);

	static UBiomesSpawnManager* GetSpawnManager(const UActorComponent* Component);
	static UBiomesSpawnManager* GetSpawnManager(const AActor* Actor);
	
	static int32 GetInteger32Attribute(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName AttributeName);
	static FName GetNameAttribute(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName AttributeName);
	
	template <typename AttributeType>
	static bool CreateAndSetAttribute(FName AttributeName, UPCGMetadata* Metadata, AttributeType Value)
	{
		FPCGMetadataAttribute<AttributeType>* NewAttribute = static_cast<FPCGMetadataAttribute<AttributeType>*>(
			Metadata->CreateAttribute<AttributeType>(AttributeName, Value, /*bAllowInterpolation=*/false, /*bOverrideParent=*/false));
		
		if (!NewAttribute)
		{
			return false;
		}
		
		NewAttribute->SetValue(Metadata->AddEntry(), Value);
		return true;
	}
	
	template<typename T>
	static void SetAttributeHelper(UPCGSpatialData* Data, const FPCGAttributePropertySelector& PropertySelector, const TArrayView<const T> Values)
	{
		if (PropertySelector.GetSelection() == EPCGAttributePropertySelection::Attribute)
		{
			// Discard None attributes
			const FName AttributeName = PropertySelector.GetAttributeName();
			if (AttributeName == NAME_None)
			{
				return;
			}
			constexpr bool Interpolate = std::is_arithmetic_v<T>;
			Data->Metadata->FindOrCreateAttribute<T>(AttributeName, {}, Interpolate);
		}

		TUniquePtr<IPCGAttributeAccessor> Accessor = PCGAttributeAccessorHelpers::CreateAccessor(Data, PropertySelector);
		if (!Accessor)
		{
			return;
		}

		TUniquePtr<IPCGAttributeAccessorKeys> Keys = PCGAttributeAccessorHelpers::CreateKeys(Data, PropertySelector);
		if (!Keys)
		{
			return;
		}

		Accessor->SetRange(Values, 0, *Keys);
	}
};
