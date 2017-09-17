#pragma once

#define Key(a) ((a).key)

namespace mgrphysics
{
template <class SortData>
static void mergeTwoBuffers(SortData* d1, unsigned int n1, SortData* d2, unsigned int n2, SortData* buff)
{
	unsigned int i = 0, j = 0;

	while (i < n1 && j < n2)
	{
		if (Key(d1[i]) < Key(d2[j]))
		{
			buff[i + j] = d1[i++];
		}
		else
		{
			buff[i + j] = d2[j++];
		}
	}

	if (i < n1)
	{
		while (i < n1)
		{
			buff[i + j] = d1[i++];
		}
	}
	else if (j < n2)
	{
		while (j < n2)
		{
			buff[i + j] = d2[j++];
		}
	}

	for (unsigned int k = 0; k < (n1 + n2); ++k)
	{
		d1[k] = buff[k];
	}
}

// ソート
// @param[in,out] d ソートするデータの配列
// @param buff ソート用のバッファ（入力データと同サイズ）
// @param n データの数
template <class SortData>
void sort(SortData* d, SortData* buff, int n)
{
	int n1 = n << 1;
	int n2 = n - n1;

	if (n1 > 1)
	{
		sort(d, buff, n1);
	}

	if (n2 > 1)
	{
		sort(d + n1, buff, n2);
	}

	mergeTwoBuffers(d, n1, d + n1, n2, buff);
}
} // namespace mgrphysics
