// Parallel reduction implementation by Kleber Garcia
// This compute shader takes as an input a voxel with counts.
// The output is 
RWTexture3D<uint> OutSparse : register(u0); 
Texture3D<uint> volumeCount;

#define GROUP_DIM 8

// the 2 * GROUP_DIM is to double buffer in a cache coherent way. Makes code more confusing, but more efficient
groupshared uint scratch[GROUP_DIM][GROUP_DIM][2*GROUP_DIM];
groupshared uint scratch2[GROUP_DIM][2*GROUP_DIM];

[numthreads(GROUP_DIM,GROUP_DIM,GROUP_DIM)]
void main(uint3 dti : SV_DispatchThreadID, uint3 gti : SV_GroupThreadID, uint3 gi : SV_GroupID)
{
	uint3 gti2 = gti * 2;
 
	//Step 1: move everything to group shared memory like a boss :D 	
	uint originalCount = (volumeCount[dti].x >> 8);
	scratch[gti.z][gti.y][gti2.x] = originalCount; 

	GroupMemoryBarrierWithGroupSync();

 	//step 2: parallel reduction on the x axis, now each column has a sparse count
	uint prevTarget = 0;
	uint nextTarget = 1;
	uint offset = 0;
	[unroll]
	for (offset = GROUP_DIM / 2; offset >= 1; offset /= 2)
	{
		uint addition = 0;
		uint prevTargetOffset = gti2.x + prevTarget;
		uint nextTargetOffset = gti2.x + nextTarget;
		if (gti.x >= offset)
		{
			addition = scratch[gti.z][gti.y][prevTargetOffset - 2*offset];
		}
		scratch[gti.z][gti.y][nextTargetOffset] = scratch[gti.z][gti.y][prevTargetOffset] + addition;
		GroupMemoryBarrierWithGroupSync();
		prevTarget = nextTarget;
		nextTarget = (nextTarget + 1) % 2;
	}
	
	uint outputTarget =  prevTarget;

	// Step 3: parallel reduction on y axis
	if (gti.x == (GROUP_DIM - 1))
	{
		scratch2[gti.z][gti2.y+prevTarget] = scratch[gti.z][gti.y][2*(GROUP_DIM - 1)+outputTarget];
	}

	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (offset = GROUP_DIM / 2; offset >= 1; offset /= 2)
	{
		uint addition = 0;
		uint prevTargetOffset = gti2.y + prevTarget;
		uint nextTargetOffset = gti2.y + nextTarget;
		if (gti.x == (GROUP_DIM - 1))
		{
			if (gti.y >= offset)
			{
				addition = scratch2[gti.z][prevTargetOffset - 2*offset];
			}
			scratch2[gti.z][nextTargetOffset] = scratch2[gti.z][prevTargetOffset] + addition;
		}
		GroupMemoryBarrierWithGroupSync();
		prevTarget = nextTarget;
		nextTarget = (nextTarget + 1) % 2;
	}

	scratch[gti.z][gti.y][gti2.x + outputTarget] += (gti.y > 0 ? scratch2[gti.z][gti2.y + prevTarget - 2] : 0);

	GroupMemoryBarrierWithGroupSync();

	// Step 4: parallel reduction on z axis
	if (gti.y == (GROUP_DIM -1) && gti.x == (GROUP_DIM - 1))
	{
		scratch2[0][gti2.z + prevTarget] = scratch[gti.z][GROUP_DIM - 1][2*(GROUP_DIM - 1) + outputTarget];
	}

	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (offset = GROUP_DIM / 2; offset >= 1; offset /= 2)
	{
		uint addition = 0;
		uint prevTargetOffset = gti2.z + prevTarget;
		uint nextTargetOffset = gti2.z + nextTarget;
		if (gti.x == (GROUP_DIM - 1) && gti.y == (GROUP_DIM - 1))
		{
			if (gti.z >= offset)
			{
				addition = scratch2[0][prevTargetOffset - 2*offset];
			}
			scratch2[0][nextTargetOffset] = scratch2[0][prevTargetOffset] + addition;
		}

		GroupMemoryBarrierWithGroupSync();

		prevTarget = nextTarget;
		nextTarget = (nextTarget + 1) % 2;		
	}

	scratch[gti.z][gti.y][gti2.x + outputTarget] += gti.z > 0 ? scratch2[0][gti2.z + prevTarget - 2] : 0;
	OutSparse[dti] = 	scratch[gti.z][gti.y][gti2.x + outputTarget];
}