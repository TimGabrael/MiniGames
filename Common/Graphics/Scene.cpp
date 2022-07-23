#include "Scene.h"
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>


#include "logging.h"

#define SCENE_ALLOCATION_TYPES_STEPS 20

#define NUM_ELEMENTS_IN_BATCH_LIST64 64
struct PtrSafeBatchListHeader64
{
	PtrSafeBatchListHeader64* prev;
	PtrSafeBatchListHeader64* next;
	uint64_t usedbitmask;
};


static int PositionOfRightMostClearedBit(uint64_t testMask)
{
	if (testMask == UINT64_MAX)
		return -1;

	uint64_t cur = 1;
	int bitPos = 1;
	while (cur & testMask)
	{
		bitPos = bitPos + 1;
		cur = cur << 1;
	}
	return bitPos;
}

static PtrSafeBatchListHeader64* CreateBatchList(uint32_t elementSize)
{
	const uint32_t size = NUM_ELEMENTS_IN_BATCH_LIST64 * elementSize + sizeof(PtrSafeBatchListHeader64);
	PtrSafeBatchListHeader64* list = (PtrSafeBatchListHeader64*)malloc(size);
	assert(list != nullptr);
	memset(list, 0, size);
	return list;
}
static void FreeBatchList(PtrSafeBatchListHeader64* list)
{
	assert(list != nullptr);
	free(list);
}

static void* AddElementToBatchList(PtrSafeBatchListHeader64* list, uint32_t elementSize)
{
	assert(list != nullptr);
	if (list->usedbitmask == UINT64_MAX)
	{
		if (list->next)
		{
			return AddElementToBatchList(list->next, elementSize);
		}
		else
		{
			list->next = CreateBatchList(elementSize);
			list->next->prev = list;
			int freeBit = PositionOfRightMostClearedBit(list->usedbitmask);
			list->usedbitmask |= (1LL << (freeBit - 1));
			int listIdx = elementSize * (freeBit - 1);
			return (void*)((uintptr_t)list + listIdx + sizeof(PtrSafeBatchListHeader64));
		}
	}
	else
	{
		int freeBit = PositionOfRightMostClearedBit(list->usedbitmask);
		list->usedbitmask |= (1LL << (freeBit - 1));
		int listIdx = elementSize * (freeBit - 1);
		return (void*)((uintptr_t)list + listIdx + sizeof(PtrSafeBatchListHeader64));
	}
}
static bool RemoveFromList(PtrSafeBatchListHeader64* list, void* ptr, uint32_t elementSize)
{
	assert(list != nullptr);
	const uintptr_t start = (uintptr_t)list + sizeof(PtrSafeBatchListHeader64);
	const uintptr_t end = start + NUM_ELEMENTS_IN_BATCH_LIST64 * elementSize;
	if (start <= (uintptr_t)ptr && (uintptr_t)ptr < end)
	{
		uintptr_t ptrSzIdx = (uintptr_t)ptr - start;
		if (ptrSzIdx % elementSize) return false;	// the pointer points in the middle of the object
		
		uint64_t removeBit = 1LL << (ptrSzIdx / elementSize);
		list->usedbitmask &= ~removeBit;

		if((list->usedbitmask == 0) && list->prev)
		{
			list->prev->next = list->next;
			FreeBatchList(list);
		}

		return true;
	}
	else if ((uint64_t)ptr >= end && list->next)
	{
		return RemoveFromList(list->next, ptr, elementSize);
	}
	else if((uint64_t)ptr < start && list->prev)
	{
		return RemoveFromList(list->prev, ptr, elementSize);
	}
	return false;
}
static void DeleteList(PtrSafeBatchListHeader64* list)
{
	PtrSafeBatchListHeader64* cur = list;
	while (cur->prev)
	{
		cur = cur->prev;
	}
	while (cur)
	{
		PtrSafeBatchListHeader64* next = cur->next;
		FreeBatchList(list);
		cur = next;
	}
}
static void* GetElementFromList(PtrSafeBatchListHeader64* list, uint32_t idx, uint32_t elementSize)
{
	if (list->usedbitmask & (1LL << idx)) return (void*)(((uintptr_t)list) + sizeof(PtrSafeBatchListHeader64) + idx * elementSize);
	return nullptr;
}



struct _ImplScene
{
	PtrSafeBatchListHeader64** objectDatas;


	PtrSafeBatchListHeader64** meshes;
	PtrSafeBatchListHeader64** materials;
	PtrSafeBatchListHeader64** transforms;

	PFUNCGETDRAWFUNCTION* functions;

	int numTypes;
	int capacityTypes;

	int numObjects;
};



PScene SC_CreateScene()
{
	_ImplScene* scene = (_ImplScene*)malloc(sizeof(_ImplScene));
	assert(scene != nullptr);
	memset(scene, 0, sizeof(_ImplScene));
	return scene;
}
uint32_t SC_AddType(PScene scene, PFUNCGETDRAWFUNCTION function)
{
	assert(scene != nullptr && function != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (sc->numTypes + 1 >= sc->capacityTypes)
	{
		sc->capacityTypes = sc->capacityTypes + SCENE_ALLOCATION_TYPES_STEPS;
		{
			PFUNCGETDRAWFUNCTION* funcs = (PFUNCGETDRAWFUNCTION*)malloc(sc->capacityTypes * sizeof(PFUNCGETDRAWFUNCTION));
			memset(funcs, 0, sc->capacityTypes * sizeof(PFUNCGETDRAWFUNCTION));
			if (sc->functions)
			{
				memcpy(funcs, sc->functions, sc->numTypes * sizeof(PFUNCGETDRAWFUNCTION));
				free(sc->functions);
			}
			sc->functions = funcs;
		}
		{
			PtrSafeBatchListHeader64** objs = (PtrSafeBatchListHeader64**)malloc(sc->capacityTypes * sizeof(void*));
			memset(objs, 0, sc->capacityTypes * sizeof(void*));
			if (sc->objectDatas)
			{
				memcpy(objs, sc->objectDatas, sc->numTypes * sizeof(void*));
				free(sc->objectDatas);
			}
			sc->objectDatas = objs;
		}
		{
			PtrSafeBatchListHeader64** meshes = (PtrSafeBatchListHeader64**)malloc(sc->capacityTypes * sizeof(void*));
			memset(meshes, 0, sc->capacityTypes * sizeof(void*));
			if (sc->meshes) 
			{
				memcpy(meshes, sc->meshes, sc->numTypes * sizeof(void*));
				free(sc->meshes);
			}
			sc->meshes = meshes;
		}
		{
			PtrSafeBatchListHeader64** materials = (PtrSafeBatchListHeader64**)malloc(sc->capacityTypes * sizeof(void*));
			memset(materials, 0, sc->capacityTypes * sizeof(void*));
			if (sc->materials)
			{
				memcpy(materials, sc->materials, sc->numTypes * sizeof(void*));
				free(sc->materials);
			}
			sc->materials = materials;
		}
		{
			PtrSafeBatchListHeader64** transforms = (PtrSafeBatchListHeader64**)malloc(sc->capacityTypes * sizeof(void*));
			memset(transforms, 0, sc->capacityTypes * sizeof(void*));
			if (sc->transforms)
			{
				memcpy(transforms, sc->transforms, sc->numTypes * sizeof(void*));
				free(sc->transforms);
			}
			sc->transforms = transforms;
		}
	}
	sc->functions[sc->numTypes] = function;
	sc->numTypes = sc->numTypes + 1;
	return sc->numTypes - 1;
}
PMaterial SC_AddMaterial(PScene scene, uint32_t typeIndex, uint32_t materialSize)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return nullptr;
	if (!sc->materials[typeIndex]) sc->materials[typeIndex] = CreateBatchList(materialSize);
	return AddElementToBatchList(sc->materials[typeIndex], materialSize);
}
PMesh SC_AddMesh(PScene scene, uint32_t typeIndex, uint32_t meshSize)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return nullptr;
	if (!sc->meshes[typeIndex]) sc->meshes[typeIndex] = CreateBatchList(meshSize);
	return AddElementToBatchList(sc->meshes[typeIndex], meshSize);
}
PTransform SC_AddTransform(PScene scene, uint32_t typeIndex, uint32_t transformSize)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return nullptr;
	if (!sc->transforms[typeIndex]) sc->transforms[typeIndex] = CreateBatchList(transformSize);
	return AddElementToBatchList(sc->transforms[typeIndex], transformSize);
}
SceneObject* SC_AddSceneObject(PScene scene, uint32_t typeIndex)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return nullptr;
	if (!sc->objectDatas[typeIndex]) sc->objectDatas[typeIndex] = CreateBatchList(sizeof(SceneObject));
	SceneObject* obj = (SceneObject*)AddElementToBatchList(sc->objectDatas[typeIndex], sizeof(SceneObject));
	if (obj) sc->numObjects = sc->numObjects + 1;
	return obj;
}



void SC_RemoveType(PScene scene, uint32_t typeIndex)	// TODO: IM NOT TO SURE WHAT TO DO IN HERE, ( SHOULD THE ARRAY BE RECENTERED TO FILL IN THE HOLE CREATED BY THIS FUNCTION? )
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return;
}
void SC_RemoveMaterial(PScene scene, uint32_t typeIndex, uint32_t materialSize, PMaterial rmMaterial)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return;
	RemoveFromList(sc->materials[typeIndex], rmMaterial, materialSize);
}
void SC_RemoveMesh(PScene scene, uint32_t typeIndex, uint32_t meshSize, PMesh rmMesh)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return;
	RemoveFromList(sc->materials[typeIndex], rmMesh, meshSize);
}
void SC_RemoveTransform(PScene scene, uint32_t typeIndex, uint32_t transformSize, PTransform rmTransform)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return;
	RemoveFromList(sc->materials[typeIndex], rmTransform, transformSize);
}
void SC_RemoveSceneObject(PScene scene, uint32_t typeIndex, SceneObject* rmObj)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (typeIndex >= sc->numTypes) return;
	if (RemoveFromList(sc->materials[typeIndex], rmObj, sizeof(SceneObject)))
	{
		sc->numObjects = sc->numObjects - 1;
	}
}




















bool CubeCubeIntersectionTest(const BoundingBox* bbox1, const BoundingBox* bbox2)
{
	const glm::vec3* min1 = &bbox1->leftTopFront;
	const glm::vec3* max1 = &bbox1->rightBottomBack;
	const glm::vec3* min2 = &bbox2->leftTopFront;
	const glm::vec3* max2 = &bbox2->rightBottomBack;

	return	(min1->x < max2->x && max1->x > min2->x) &&
			(min1->y < max2->y && max1->y > min2->y) &&
			(min1->z < max2->z && max1->z > min2->z);
}
void SortBoundingBox(BoundingBox* b)
{
	glm::vec3* min = &b->leftTopFront;
	glm::vec3* max = &b->rightBottomBack;
	float temp;
	if (min->x > max->x) {
		temp = max->x;
		max->x = min->x; min->x = temp;
	}
	if (min->y > max->y) {
		temp = max->y;
		max->y = min->y; min->y = temp;
	}
	if (min->z > max->z) {
		temp = max->z;
		max->z = min->z; min->z = temp;
	}
}
void GenerateBBoxView(BoundingBox* view, const glm::mat4* viewProjMatrix)
{
	glm::mat4 invMat = glm::inverse(*viewProjMatrix);

	glm::vec4 tlf = { -1.0f, -1.0f, -1.0f, 1.0f }; glm::vec4 trf = { 1.0f, -1.0f, -1.0f, 1.0f };
	glm::vec4 blf = { -1.0f, 1.0f, -1.0f, 1.0f }; glm::vec4 brf = { 1.0f, 1.0f, -1.0f, 1.0f };

	glm::vec4 tlb = { -1.0f, -1.0f, 1.0f, 1.0f }; glm::vec4 trb = { 1.0f, -1.0f, 1.0f, 1.0f };
	glm::vec4 blb = { -1.0f, 1.0f, 1.0f, 1.0f }; glm::vec4 brb = { 1.0f, 1.0f, 1.0f, 1.0f };

#define SET_AS_MIN_MAX(vec) if(vec.x < min.x) min.x = vec.x;\
							if(vec.y < min.y) min.y = vec.y;\
							if(vec.z < min.z) min.z = vec.z;\
							if(max.x < vec.x) max.x = vec.x;\
							if(max.y < vec.y) max.y = vec.y;\
							if(max.z < vec.z) max.z = vec.z;

	tlf = invMat * tlf; trf = invMat * trf;
	blf = invMat * blf; brf = invMat * brf;

	tlb = invMat * tlb; trb = invMat * trb;
	blb = invMat * blb; brb = invMat * brb;

	tlf /= tlf.w; trf /= trf.w; blf /= blf.w; brf /= brf.w;
	tlb /= tlb.w; trb /= trb.w; blb /= blb.w; brb /= brb.w;

	glm::vec3 min = tlf; glm::vec3 max = tlf;

	SET_AS_MIN_MAX(trf); SET_AS_MIN_MAX(blf); SET_AS_MIN_MAX(brf);
	SET_AS_MIN_MAX(tlb); SET_AS_MIN_MAX(trb); SET_AS_MIN_MAX(blb);
	SET_AS_MIN_MAX(brb);


#undef SET_AS_MIN_MAX

	*view = { min, max };

}



ObjectRenderStruct* SC_GenerateRenderList(PScene scene)
{
	_ImplScene* sc = (_ImplScene*)scene;
	ObjectRenderStruct* output = (ObjectRenderStruct*)malloc(sc->numObjects * sizeof(ObjectRenderStruct));

	return output;
}
ObjectRenderStruct* SC_FillRenderList(PScene scene, ObjectRenderStruct* list, const glm::mat4* viewProjMatrix, int* num, TYPE_FUNCTION f, uint32_t objFlag)
{
	assert(scene != nullptr && viewProjMatrix != nullptr && num != nullptr && list != nullptr && f < TYPE_FUNCTION::NUM_TYPE_FUNCTION && objFlag != 0);
	_ImplScene* sc = (_ImplScene*)scene;
	BoundingBox viewSpace;
	GenerateBBoxView(&viewSpace, viewProjMatrix);

	int count = 0;
	for (int i = 0; i < sc->numTypes; i++)
	{
		PtrSafeBatchListHeader64* cur = sc->objectDatas[i];
		PFUNCDRAWSCENEOBJECT func = sc->functions[i](f);
		if (!func) continue;
		while (cur)
		{
			for (int j = 0; j < NUM_ELEMENTS_IN_BATCH_LIST64; j++)
			{
				SceneObject* sObj = (SceneObject*)GetElementFromList(cur, j, sizeof(SceneObject));
				if (sObj)
				{
					if (sObj->base.flags & objFlag)
					{
						if (CubeCubeIntersectionTest(&viewSpace, &sObj->base.bbox))
						{
							list[count].obj = sObj;
							list[count].DrawFunc = func;
							count++;
						}
					}
				}
			}
			cur = cur->next;
		}
	}
	*num = count;
	return list;
}



void SC_FreeRenderList(ObjectRenderStruct* list)
{
	if (list)
	{
		free(list);
	}
}