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
#include "LBBaseBiomeLayer.h"
#include "LBGenericBiomeLayer.generated.h"


/**
 * 
 */
UCLASS()
class PCGLAYEREDBIOMES_API ULBGenericBiomeLayer : public ULBBaseBiomeLayer
{
	GENERATED_BODY()
	
public:
	ULBGenericBiomeLayer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion")
	ELBBiomeLayerExclusion InExclusion = ELBBiomeLayerExclusion::Points;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion", meta=(EditCondition="InExclusion==ELBBiomeLayerExclusion::MeshBounds"))
	EPCGBoundsModifierMode InExclusionBoundsMode = EPCGBoundsModifierMode::Scale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion", meta=(EditCondition="InExclusion==ELBBiomeLayerExclusion::MeshBounds"))
	FVector InExclusionBoundsValue = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion")
	ELBBiomeLayerExclusion OutExclusion = ELBBiomeLayerExclusion::MeshBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion", meta=(EditCondition="OutExclusion==ELBBiomeLayerExclusion::MeshBounds"))
	EPCGBoundsModifierMode OutExclusionBoundsMode = EPCGBoundsModifierMode::Scale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exclusion", meta=(EditCondition="OutExclusion==ELBBiomeLayerExclusion::MeshBounds"))
	FVector OutExclusionBoundsValue = FVector::OneVector * 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	double Density = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	int Seed = 0;

	/**
	 * Enable filtering of points based on random noise.
	 * To visualize noise use Custom1 debug mode for the layer 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	bool UseNoise = true;

	/**
	 * Scale of noise.
	 * Lower values - lower frequency 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(EditCondition="UseNoise", EditConditionHides="UseNoise"))
	double NoiseScale = 8;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(EditCondition="UseNoise", EditConditionHides="UseNoise"))
	double NoiseFilterLow = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(EditCondition="UseNoise", EditConditionHides="UseNoise"))
	double NoiseFilterHigh = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(EditCondition="UseNoise", EditConditionHides="UseNoise"))
	double NoiseSeed = 10000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(InlineEditConditionToggle))
	bool NoSlopesEnabled = false;

	/**
	 * Disable spawning on slopes with a slope greater than specified value
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn", meta=(EditCondition="NoSlopesEnabled"))
	float NoSlopesValue = 0.25f;

	/**
	 * Force points to be strictly vertical
	 * Otherwise points match landscape normal 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	bool AbsoluteRotation = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	double OffsetZ = 0;

	/**
	 * Horizontal distance to randomly shift points
	 * Value 100 means randomize position in 1m radius around initial position  
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	float TransformRange = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	float ScaleMin = 0.8f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spawn")
	float ScaleMax = 1.2f;

	/**
	 * PCG Graph to spawn meshes. Can be omitted.
	 * Typically, it should contain only a StaticMeshSpawner graph node with specific mesh template.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", AdvancedDisplay)
	TObjectPtr<UPCGGraphInterface> SpawnerGraph;

protected:
	FRandomStream Stream;
};
