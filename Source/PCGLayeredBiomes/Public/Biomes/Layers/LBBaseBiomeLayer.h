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
#include "Elements/PCGBoundsModifier.h"
#include "UObject/Object.h"
#include "LBBaseBiomeLayer.generated.h"

class UPCGGraph;
class UPCGGraphInterface;

UENUM()
enum class ELBBiomeLayerExclusion : uint8
{
	/**
	 * Output bounds will be calculated from mesh bounds 
	 */
	MeshBounds,
	
	/**
	 * Extents of points (1x1m) will be used as a bounds
	 */
	Points,
	
	/**
	 * Ignore bounds of the layer and allow to overlap other layers with it 
	 */
	DontExclude,
};

UENUM()
enum class ELBBiomeLayerDebugMode
{
	/**
	 * Show transformed points
	 * (before overlapping with avoid regions)
	 */
	Points,
	
	/**
	 * Show transformed points with mesh bounds applied
	 * (before overlapping with avoid regions)
	 */
	MeshBounds,
	
	/**
	 * Show input avoid regions
	 */
	AvoidRegions,

	/**
	 * Show exclusion regions which will be used to check with avoid regions
	 */
	InExclusion,
	
	/**
	 * Show outgoing exclusion regions of spawned objects
	 */
	OutExclusion,

	/**
	 * Can be used in graph to show other debugging information
	 */
	Custom1,
	/**
	 * Can be used in graph to show other debugging information
	 */
	Custom2,
};

/**
 * 
 */
UCLASS(Abstract, Const, DefaultToInstanced, EditInlineNew, Blueprintable, AutoExpandCategories=(Default), PrioritizeCategories=(Default))
class PCGLAYEREDBIOMES_API ULBBaseBiomeLayer : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Disabled layers doesn't generate content 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default", meta=(DisplayPriority=-3))
	bool Enabled = true;

	/**
	 * Could be used in Layer PCG Graph to draw some debug information  
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default", meta=(InlineEditConditionToggle))
	bool DebugEnabled = false;

	/**
	 * Could be used in Layer PCG Graph to draw some debug information  
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default", meta=(DisplayPriority=-2, EditCondition=DebugEnabled))
	ELBBiomeLayerDebugMode Debug = ELBBiomeLayerDebugMode::Points;

	/**
	 * Main PCG Graph of the layer. Must be specified to generate anything.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default", AdvancedDisplay)
	TObjectPtr<UPCGGraphInterface> Graph;

	/**
	 * Name of the spawn set in SpawnPreset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default", meta=(DisplayPriority=-1))
	FName SpawnSet;
};
