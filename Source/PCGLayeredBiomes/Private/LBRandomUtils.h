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
#include "Math/RandomStream.h"

template <typename T>
struct TFLBWeightGetter
{
	static constexpr int GetWeight(const T& Item)
	{
		if constexpr (std::is_pointer_v<T>)
		{
			return Item->Weight;
		}
		else
		{
			return Item.Weight;
		}
	}
};

class FLBRandomUtils
{
public:
	template <typename TItem>
	static int SelectRandomIndex(const TArray<TItem>& Items, const FRandomStream& RandomSource, const int* InTotalWeight = nullptr)
	{
		if (Items.IsEmpty())
		{
			return INDEX_NONE;
		}
		
		int TotalWeight = 0;
		if (InTotalWeight)
		{
			// Use total weight from input
			TotalWeight = *InTotalWeight;
		}
		else
		{
			// Calculate total weight
			for (auto&& Item: Items)
			{
				TotalWeight += TFLBWeightGetter<TItem>::GetWeight(Item);
			}
		}
		
		if (TotalWeight <= 1)
		{
			return Items.Num() - 1;	
		}
		
		const auto RandomWeight = RandomSource.RandRange(0, TotalWeight - 1);
		int CurWeight = 0;
		for (int i = 0; i < Items.Num(); ++i)
		{
			CurWeight += TFLBWeightGetter<TItem>::GetWeight(Items[i]);
			if (CurWeight > RandomWeight)
			{
				return i;
			}
		}

		return Items.Num() - 1;
	}
	
	template <typename TItem>
	static const TItem& SelectRandom(const TArray<TItem>& Items, const FRandomStream& RandomSource, const int* InTotalWeight = nullptr)
	{
		const auto Index = SelectRandomIndex(Items, RandomSource, InTotalWeight);
		check(Index != INDEX_NONE);
		return Items[Index];
	}

};
