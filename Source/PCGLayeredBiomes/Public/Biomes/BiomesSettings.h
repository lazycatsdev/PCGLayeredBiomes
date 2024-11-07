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
#include "PCGCrc.h"
#include "UObject/Object.h"
#include "Engine/DataAsset.h"
#include "BiomesSettings.generated.h"

class UPCGBiomesBaseFilter;
class UPCGMetadata;
struct FPCGPoint;
struct FBiomeFilter;
class UPCGGraph;
class UBaseBiomeLayer;

/**
 * 
 */
USTRUCT(BlueprintType)
struct PCGLAYEREDBIOMES_API FBiomeSettings
{
	GENERATED_BODY()

	/**
	 * Disabled biomes doesn't generate any content
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category=Biomes)
	bool Enabled = true;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category=Biomes, meta=(InlineEditConditionToggle))
	bool Debug = false;
	/**
	 * Draw mask of the biome with specified color
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category=Biomes, AdvancedDisplay, meta=(EditCondition=Debug))
	FLinearColor DebugColor = FLinearColor::Transparent;

	/**
	 * Sets priority of the biome. If landscape point can passed filters for several biomes,
	 * biome with lower priority take precedence.  
	 */
	UPROPERTY(EditDefaultsOnly, Category=Biomes)
	int32 Priority = 5000;
	
	/**
	 * All filters should pass (return `true`) for a point to include the point to the biome.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category=Biomes)
	TArray<TObjectPtr<UPCGBiomesBaseFilter>> Filters;

	/**
	 * Content generation layers.
	 * Executed in specified order.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category=Biomes, meta=(TitleProperty = "SpawnSet"))
	TArray<TObjectPtr<UBaseBiomeLayer>> Layers;
};

USTRUCT()
struct PCGLAYEREDBIOMES_API FBiomeSettings_Named : public FBiomeSettings
{
	GENERATED_BODY()

	FBiomeSettings_Named() = default;
	FBiomeSettings_Named(FName InName, FBiomeSettings InSettings);
	
	UPROPERTY(EditDefaultsOnly, Category=Biomes)
	FName Name;
};

UCLASS(BlueprintType, Transient, ClassGroup=(Biomes))
class PCGLAYEREDBIOMES_API UBiomesData : public UObject
{
	GENERATED_BODY()

	friend class UBiomesSettings;
	
public:
	UFUNCTION(BlueprintCallable, Category=Biomes)
	bool DetectBiome(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName& OutBiome, int& OutPriority) const;

protected:
	UPROPERTY(Transient)
	TArray<FBiomeSettings_Named> Biomes;
};

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup=(Biomes))
class PCGLAYEREDBIOMES_API UBiomesSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	FPCGCrc ComputeCrc();

	UFUNCTION(BlueprintCallable, Category=Biomes)
	UBiomesData* Prepare() const; 

	const FBiomeSettings* FindSettings(FName Name) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Biomes)
	TMap<FName, FBiomeSettings> Biomes;
};
