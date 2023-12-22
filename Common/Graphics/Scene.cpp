#include "Scene.h"
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>
#include <vector>
#include <algorithm>


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
    std::vector<SceneObject*> objects;

	PtrSafeBatchListHeader64* pointLights;
	PtrSafeBatchListHeader64* directionalLights;

	int numPointLights = 0;
	int numDirectionalLights = 0;
};



PScene SC_CreateScene()
{
	_ImplScene* scene = new _ImplScene();
	scene->pointLights = CreateBatchList(sizeof(PointLightData));
	scene->directionalLights = CreateBatchList(sizeof(DirectionalLightData));
	return scene;
}
void SC_DestroyScene(PScene scene) {
    _ImplScene* s = (_ImplScene*)scene;
    DeleteList(s->pointLights);
    DeleteList(s->directionalLights);
    delete s;
}


void SC_AddSceneObject(PScene scene, SceneObject* obj)
{
    assert(scene != nullptr);
    _ImplScene* s = (_ImplScene*)scene;
    s->objects.push_back(obj);
    std::sort(s->objects.begin(), s->objects.end(), [](SceneObject* o1, SceneObject* o2) {
            size_t t1 = o1->GetType();
            size_t t2 = o2->GetType();
            if(t1 < t2) return 1;
            else if(t1 > t2) return -1;
            return 0;
    });
}

ScenePointLight* SC_AddPointLight(PScene scene)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	ScenePointLight* out = (ScenePointLight*)AddElementToBatchList(sc->pointLights, sizeof(ScenePointLight));
	if (out)
	{
		sc->numPointLights = sc->numPointLights + 1;
		out->group = 1;
	}
	return out;
}
SceneDirLight* SC_AddDirectionalLight(PScene scene)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	SceneDirLight* out = (SceneDirLight*)AddElementToBatchList(sc->directionalLights, sizeof(SceneDirLight));
	if (out)
	{
		sc->numDirectionalLights = sc->numDirectionalLights + 1;
		out->group = 1;
	}
	return out;
}

void SC_RemoveSceneObject(PScene scene, SceneObject* rmObj)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
    for(size_t i = 0; i < sc->objects.size(); i++) {
        if(sc->objects.at(i) == rmObj) {
            sc->objects.erase(sc->objects.begin() + i);
            break;
        }
    }
}



void SC_RemovePointLight(PScene scene, ScenePointLight* light)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (RemoveFromList(sc->pointLights, light, sizeof(ScenePointLight)))
	{
		sc->numPointLights = sc->numPointLights - 1;
	}
}
void SC_RemoveDirectionalLight(PScene scene, SceneDirLight* light)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	if (RemoveFromList(sc->directionalLights, light, sizeof(SceneDirLight)))
	{
		sc->numDirectionalLights = sc->numDirectionalLights - 1;
	}
}


int SC_GetNumLights(PScene scene)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	return sc->numPointLights + sc->numDirectionalLights;
}

void SC_FillLights(PScene scene, ScenePointLight** pl, SceneDirLight** dl)
{
	assert(scene != nullptr);
	_ImplScene* sc = (_ImplScene*)scene;
	for (int i = 0; i < sc->numPointLights; i++)
	{
		pl[i] = (ScenePointLight*)GetElementFromList(sc->pointLights, i, sizeof(ScenePointLight));
	}
	for (int i = 0; i < sc->numDirectionalLights; i++)
	{
		dl[i] = (SceneDirLight*)GetElementFromList(sc->directionalLights, i, sizeof(SceneDirLight));
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



SceneObject** SC_GenerateRenderList(PScene scene)
{
	_ImplScene* sc = (_ImplScene*)scene;
	SceneObject** output = (SceneObject**)malloc(sc->objects.size() * 2 * sizeof(void*));	// twice as big for both opaque and blended objects
	return output;
}
SceneObject** SC_FillRenderList(PScene scene, SceneObject** list, const glm::mat4* viewProjMatrix, int* num, uint32_t objFlag)
{
	assert(scene != nullptr && viewProjMatrix != nullptr && num != nullptr && list != nullptr && objFlag != 0);
	_ImplScene* sc = (_ImplScene*)scene;
	BoundingBox viewSpace;
	GenerateBBoxView(&viewSpace, viewProjMatrix);

    int count = 0;
    for(SceneObject* obj : sc->objects) {
        if((obj->flags & objFlag) == objFlag) {
            if(CubeCubeIntersectionTest(&viewSpace, &obj->bbox)) {
                list[count] = obj;
                count++;
            }
        }
    }
	*num = count;
	return list;
}
SceneObject** SC_AppendRenderList(PScene scene, SceneObject** list, const glm::mat4* viewProjMatrix, int* num, uint32_t objFlag, int curNum)
{
	assert(scene != nullptr && viewProjMatrix != nullptr && num != nullptr && list != nullptr && objFlag != 0);
	_ImplScene* sc = (_ImplScene*)scene;
	BoundingBox viewSpace;
	GenerateBBoxView(&viewSpace, viewProjMatrix);

	int count = curNum;
    for(SceneObject* obj : sc->objects) {
        if((obj->flags & objFlag) == objFlag) {
            if(CubeCubeIntersectionTest(&viewSpace, &obj->bbox)) {
                list[count] = obj;
                count++;
            }
        }
    }
	*num = count;
	return list;
}


void SC_FreeRenderList(SceneObject** list)
{
	if (list) {
		free(list);
	}
}
