#include "stdafx.h"
#include <cmath>

//Misc
#include "IniFile.hpp"
#include "ObjModels/Objects.h"

//Structs
struct ObjectThing
{
	ObjectFuncPtr func;
	int16_t list;
	int16_t field_A;
	Rotation3 Rotation;
	NJS_VECTOR Position;
	NJS_OBJECT* object;
};

//Additional SADX Variables
DataArray(CollisionData, stru_C67750, 0xC67750, 1);
DataArray(CollisionData, stru_C673B8, 0xC673B8, 7);

DataPointer(float, CurrentDrawDistance, 0x03ABDC74);
DataPointer(int, DroppedFrames, 0x03B1117C);

FunctionPointer(void, InitCollision, (ObjectMaster *obj, CollisionData *collisionArray, int count, unsigned __int8 list), 0x41CAF0);
FunctionPointer(void, SetStatus, (ObjectMaster *a1), 0x0049CD60);
FunctionPointer(void, DynCol_LoadObject, (ObjectMaster *a1), 0x0049E170);
FunctionPointer(void, sub_446AF0, (ObjectMaster *a1, int a2), 0x446AF0);

//Additional SADX Functions
FunctionPointer(NJS_OBJECT *, DynamicCollision, (NJS_OBJECT *a1, ObjectMaster *a2, ColFlags surfaceFlags), 0x49D6C0);
FunctionPointer(int, rand1, (), 0x6443BF);
DataPointer(int, FramerateSetting, 0x0389D7DC);
FunctionPointer(void, sub_5228A0, (ObjectMaster *a1), 0x005228A0);

SETObjData EmptyFiller = {};

//Null Code (Used for debugging purposes)
void __cdecl NullFunction(ObjectMaster *a1)
{
	return;
}

//Standard Display
void __cdecl Basic_Display(ObjectMaster *a2)
{
	EntityData1 *v1; // esi@1
	Angle v2; // eax@2
	Angle v3; // eax@4
	Angle v4; // eax@6

	v1 = a2->Data1;
	if (!MissedFrames)
	{
		SetTextureToLevelObj();
		njPushMatrix(0);
		njTranslateV(0, &v1->Position);
		v2 = v1->Rotation.z;
		if (v2)
		{
			njRotateZ(0, (unsigned __int16)v2);
		}
		v3 = v1->Rotation.x;
		if (v3)
		{
			njRotateX(0, (unsigned __int16)v3);
		}
		v4 = v1->Rotation.y;
		if (v4)
		{
			njRotateY(0, (unsigned __int16)v4);
		}
		ProcessModelNode_AB_Wrapper(v1->Object, 1.0);
		njPopMatrix(1u);
	}
}

//Standard Main
void __cdecl Basic_Main(ObjectMaster *a1)
{
	EntityData1 *v1; // edi@1

	v1 = a1->Data1;
	if (!ClipSetObject_Min(a1))
	{
		if (!ObjectSelectedDebug(a1))
		{
			AddToCollisionList(v1);
		}
		Basic_Display(a1);
	}
}

//Standard Delete Dynamic
void deleteSub_Global(ObjectMaster *a1) {
	if (a1->Data1->Object)
	{
		DynamicCOL_Remove(a1, a1->Data1->Object);
		ObjectArray_Remove(a1->Data1->Object);
	}
	DeleteObject_(a1);
}

void AddToCollision(ObjectMaster *a1, uint8_t col) {
	/*  0 is static
	1 is moving (refresh the colision every frame)
	2 is static, scalable
	3 is moving, scalable   */

	EntityData1 * original = a1->Data1;
	NJS_OBJECT *colobject;

	colobject = ObjectArray_GetFreeObject(); //The collision is a separate object, we add it to the list of object

	//if scalable
	if (col == 2 || col == 3) {
		colobject->evalflags = NJD_EVAL_BREAK | NJD_EVAL_SKIP | NJD_EVAL_HIDE;
		colobject->scl[0] = original->Scale.x;
		colobject->scl[1] = original->Scale.y;
		colobject->scl[2] = original->Scale.z;
	}
	else if (col == 4) {
		colobject->evalflags = NJD_EVAL_BREAK | NJD_EVAL_SKIP | NJD_EVAL_HIDE;
		colobject->scl[0] = 1.0f + original->Scale.x;
		colobject->scl[1] = 1.0f + original->Scale.x;
		colobject->scl[2] = 1.0f + original->Scale.x;
	}
	else if (col == 5) {
		colobject->evalflags = NJD_EVAL_BREAK | NJD_EVAL_SKIP | NJD_EVAL_HIDE;
		colobject->scl[0] = 1.0f + original->Scale.z;
		colobject->scl[1] = 1.0f + original->Scale.z;
		colobject->scl[2] = 1.0f + original->Scale.z;
	}
	else {
		colobject->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_SKIP | NJD_EVAL_HIDE; //ignore scale
		colobject->scl[0] = 1.0;
		colobject->scl[1] = 1.0;
		colobject->scl[2] = 1.0;
	}

	//add the rest
	if (col == 4 || col == 1 || col == 5)
	{
		colobject->ang[0] = 0;
		colobject->ang[1] = original->Rotation.y;
		colobject->ang[2] = 0;
	}
	else {
		colobject->ang[0] = original->Rotation.x;
		colobject->ang[1] = original->Rotation.y;
		colobject->ang[2] = original->Rotation.z;
	}
	colobject->pos[0] = original->Position.x;
	colobject->pos[1] = original->Position.y;
	colobject->pos[2] = original->Position.z;

	colobject->basicdxmodel = a1->Data1->Object->basicdxmodel; //object it will use as a collision
	a1->Data1->Object = colobject; //pointer to the collision object into our original object

	if (col == 0 || col == 2) DynamicCOL_Add((ColFlags)1, a1, colobject); //Solid
	else if (col == 1 || col == 3 || col == 4 || col == 5) DynamicCOL_Add((ColFlags)0x8000000, a1, colobject); //Dynamic, solid
}

//Basic drawing call
void DrawObjModel(ObjectMaster *a1, NJS_MODEL_SADX *m, bool scalable) {
	if (!MissedFrames) {
		njSetTexture((NJS_TEXLIST*)&BEACH01_TEXLIST); //Current heroes level texlist is always onto Emerald Coast
		njPushMatrix(0);
		njTranslateV(0, &a1->Data1->Position);
		njRotateXYZ(nullptr, a1->Data1->Rotation.x, a1->Data1->Rotation.y, a1->Data1->Rotation.z);
		if (scalable) njScale(nullptr, a1->Data1->Scale.x, a1->Data1->Scale.y, a1->Data1->Scale.z);
		else njScale(nullptr, 1, 1, 1);
		DrawQueueDepthBias = -6000.0f;
		DrawModel(m);
		DrawQueueDepthBias = 0;
		njPopMatrix(1u);
	}
}

//Egg Helicopter
void __cdecl O_Heli_Display(ObjectMaster *a2)
{
	EntityData1 *v1; // esi@1
	Angle v4; // eax@6
	Angle v5; // st7@8
	Angle v6;
	Angle v7;


	v1 = a2->Data1;
	if (!MissedFrames)
	{
		SetTextureToLevelObj();
		njPushMatrix(0);
		njTranslate(0, v1->Position.x, v1->Position.y, v1->Position.z);
		v6 = v1->Rotation.z;
		if (v6)
		{
			njRotateZ(0, (unsigned __int16)v6);
		}
		v4 = v1->Rotation.y;
		if (v4)
		{
			njRotateY(0, (unsigned __int16)v4);
		}
		v7 = v1->Rotation.x;
		if (v7)
		{
			njRotateX(0, (unsigned __int16)v7);
		}
		njAction(&action_Action_Patrol, *(float *)&v1->CharIndex);
		njPopMatrix(1u);
		if (!ObjectSelectedDebug(a2) && !IsGamePaused())
		{
			if (FramerateSetting >= 2)
			{
				*(float*)&v1->CharIndex = 2.0f + *(float*)&v1->CharIndex;
			}
			else
			{
				*(float*)&v1->CharIndex = 1.0f + *(float*)&v1->CharIndex;
			}
			if (*(float*)&v1->CharIndex >= 441.0)
			{
				*(float*)&v1->CharIndex = 0.0;
			}
		}
		/*sub_409E70((NJS_MODEL_SADX*)Patrol_Body.model, 0, 1.0);
		njPushMatrix(0);
		njTranslate(0, Patrol_Light.pos[0], (Patrol_Light.pos[1]), Patrol_Light.pos[2]);
		sub_409E70((NJS_MODEL_SADX*)Patrol_Light.model, 0, 1.0);
		njPushMatrix(0);
		njTranslate(0, Patrol_Prop.pos[0], (Patrol_Prop.pos[1]), Patrol_Prop.pos[2]);
		njRotateY(0, Patrol_Light.ang[1]);
		v5 = *(float*)&v1->CharIndex * 65536.0 * 0.002777777777777778;
		if (v5)
		{
			njRotateY(0, (unsigned __int16)v5);
		}
		sub_409E70((NJS_MODEL_SADX*)Patrol_Prop.model, 0, 1.0);
		njPopMatrix(1u);
		njPopMatrix(1u);
		njPopMatrix(1u);*/
	}
}

void __cdecl Load_O_Heli(ObjectMaster *a1)
{
	a1->MainSub = O_Heli_Display;
	a1->DisplaySub = O_Heli_Display;
	a1->Data1->Action = 0;
	a1->DeleteSub = (void(__cdecl *)(ObjectMaster *))nullsub;
}

void __cdecl HeliCheck(ObjectMaster *a1)
{
	switch (CurrentCharacter)
	{
	case 0:
		if (EventFlagArray[147] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	case 2:
		if (EventFlagArray[210] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	case 3:
		if (EventFlagArray[271] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	case 5:
		if (EventFlagArray[333] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	case 6:
		if (EventFlagArray[396] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	case 7:
		if (EventFlagArray[459] != 1)
		{
			Load_O_Heli(a1);
		}
		break;
	default:
		break;
	}
}

void __cdecl LightningCheck(ObjectMaster *a1)
{
	switch (CurrentCharacter)
	{
	case 0:
		if (EventFlagArray[147] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	case 2:
		if (EventFlagArray[210] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	case 3:
		if (EventFlagArray[271] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	case 5:
		if (EventFlagArray[333] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	case 6:
		if (EventFlagArray[396] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	case 7:
		if (EventFlagArray[459] != 1)
		{
			sub_5228A0(a1);
		}
		break;
	default:
		break;
	}
}

void LightningDrawChange(NJS_MODEL_SADX *model, float scale)
{
	DrawQueueDepthBias = 8000.0f;
	DrawModel_Queue(model, QueuedModelFlagsB_3);
	DrawQueueDepthBias = 0;
}

void __cdecl GOODModel_Display(ObjectMaster *a2)
{
	EntityData1 *v1; // esi@1
	Angle v2; // eax@2
	Angle v3; // eax@4
	Angle v4; // eax@6

	v1 = a2->Data1;
	if (!MissedFrames)
	{
		njSetTexture((NJS_TEXLIST*)&EC_TARAI_TEXLIST);
		njPushMatrix(0);
		njTranslate(0, 0, 80.0f, -510.0f);
		v2 = v1->Rotation.z;
		if (v2)
		{
			njRotateZ(0, (unsigned __int16)v2);
		}
		v3 = v1->Rotation.x;
		if (v3)
		{
			njRotateX(0, (unsigned __int16)v3);
		}
		v4 = v1->Rotation.y;
		if (v4)
		{
			njRotateY(0, (unsigned __int16)v4);
		}

		//ProcessModelNode_AB_Wrapper(&object_GOODSign, 1.0);
		ProcessModelNode(&object_GOODSign, (QueuedModelFlagsB)0, 1.0f);
		njPopMatrix(1u);
	}
}

void __cdecl GOODModel_Main(ObjectMaster *a1)
{
	EntityData1 *v1; // esi@1

	v1 = a1->Data1;
	v1->Rotation.x = 0;
	v1->Rotation.z = 0;

	if (!ClipSetObject(a1))
	{
		if (v1->Action)
		{
			if (v1->Action == 1 && !(*(float *)&v1->CharIndex <= 0))
			{
				if (!(IsGamePaused()))
				{
					*(float *)&v1->CharIndex -= 1;
				}
				GOODModel_Display(a1);
			}
			else
			{
				DeleteObjectMaster(a1);
			}
		}
		else
		{
			v1->Action = 1;
			*(float *)&v1->CharIndex = 60.0f;
			a1->DisplaySub = GOODModel_Display;
		}
	}
}

void __cdecl GOODModelRestoration()
{
	ObjectMaster *a1;
	EntityData1 *sign;
	auto entity = EntityData1Ptrs[0];

	PlaySound(12, 0, 0, 0);
	a1 = LoadObject((LoadObj)2, 3, GOODModel_Main);
	EmptyFiller.Distance = 20000.0f;
	a1->SETData.SETData = &EmptyFiller;
	if (a1)
	{
		sign = a1->Data1;
		sign->Position.x = entity->Position.x;
		sign->Position.y = entity->Position.y + 5;
		sign->Position.z = entity->Position.z;
		sign->Rotation.x = 0;
		sign->Rotation.y = 0;
		sign->Rotation.z = 0;
		sign->Scale.x = 1.0f;
		sign->Scale.y = 1.0f;
		sign->Scale.z = 1.0f;
	}
}

ObjectListEntry EggCarrierOutObjectList_list[] = {
	{ 2, 3, 0, 0, 0, Ring_Main, "RING   " } /* "RING   " */,					//00
	{ 2, 2, 0, 0, 0, Spring_Main, "SPRING " } /* "SPRING " */,					//01
	{ 2, 2, 0, 0, 0, SpringB_Main, "SPRINGB" } /* "SPRINGB" */,					//02
	{ 6, 3, 0, 0, 0, Switch_Main, "O SWITCH" } /* "O SWITCH" */,				//03
	{ 6, 3, 0, 0, 0, DashHoop_Main, "CMN_DRING" } /* "CMN_DRING" */,			//04
	{ 2, 3, 0, 0, 0, ItemBox_Main, "O ITEMBOX" } /* "O ITEMBOX" */,				//05
	{ 15, 3, 0, 0, 0, CheckPoint_Main, "O Save Point" } /* "O Save Point" */,	//06
	{ 2, 3, 0, 0, 0, Wall_Main, "WALL   " } /* "WALL   " */,					//07	
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x00525060, "SCENE CHANGE" } /* "SCENE CHANGE" */,//08
	{ 3, 3, 0, 0, 0, HeliCheck, "O HELI" } /* "O HELI" */,				//09
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x00524E10, "O HBOX" } /* "O HBOX" */,					//0A
	{ 3, 3, 1, 250000, 0, (ObjectFuncPtr)0x005248D0, "O S LIGHT" } /* "O S LIGHT" */,			//0B
	{ 2, 3, 1, 1000000, 0, (ObjectFuncPtr)0x00524370, "O EC DOOR" } /* "O EC DOOR" */,				//0C
	{ 2, 3, 1, 1000000, 0, (ObjectFuncPtr)0x00524130, "O EC DOORS" } /* "O EC DOORS" */,				//0D
	{ 2, 3, 1, 250000, 0, (ObjectFuncPtr)0x00523E60, "O TAIHOU" } /* "O TAIHOU" */,				//0E
	{ 7, 3, 0, 0, 0, (ObjectFuncPtr)0x00523340, "O SIDELIFT" } /* "O SIDELIFT" */,			//0F
	{ 6, 3, 1, 250000, 0, (ObjectFuncPtr)0x00522C00, "O TORNADO2" } /* "O TORNADO2" */,				//10
	{ 2, 3, 1, 1000000, 0, (ObjectFuncPtr)0x00522A20, "O A GATE" } /* "O A GATE" */,			//11
	{ 2, 3, 2, 0, 0, LightningCheck, "O LIGHTNING" } /* "O LIGHTNING" */,		//12
	{ 6, 3, 2, 0, 0, (ObjectFuncPtr)0x00521B30, "O EGGLIFT" } /* "O EGGLIFT" */,	//13
	{ 6, 3, 2, 0, 0, (ObjectFuncPtr)0x00521360, "O STATION" } /* "O STATION" */,					//14
	{ 2, 3, 2, 0, 0, (ObjectFuncPtr)0x00520CC0, "O MONORAIL" } /* "O MONORAIL" */,				//15
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x007A9C60, "O HINT" } /* "O HINT" */,						//16
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x004D4700, "C SPHERE" } /* "C SPHERE" */,						//17
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x004D4770, "C CYLINDER" } /* "C CYLINDER" */,						//18
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x004D47E0, "C CUBE" } /* "C CUBE" */,					//19
	{ 2, 3, 0, 0, 0, Wall_Main, "WALL   " } /* "WALL   " */,					//1A
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x004D4B70, "OTTOTTO" } /* "OTTOTTO" */,					//1B
	{ 15, 3, 0, 0, 0, (ObjectFuncPtr)0x004C07D0, "O ItemBoxAir" } /* "O ItemBoxAir" */,					//1C
	{ 6, 3, 0, 0, 0, (ObjectFuncPtr)0x00520230, "O EGGSEAT" } /* "O EGGSEAT" */,					//1D
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051FF90, "O EGGCAP" } /* "O EGGCAP" */,					//1E
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051FEE0, "O GUNSIGHT" } /* "O GUNSIGHT" */,					//1F
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051F6D0, "O DOSEI" } /* "O DOSEI" */,					//20
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051F560, "O WASHER" } /* "O WASHER" */,					//21
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051EF40, "O SLOT" } /* "O SLOT" */,					//22
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051EB50, "O LIVINGLIGHT" } /* "O LIVINGLIGHT" */,					//23
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051E910, "O ELEVATOR" } /* "O ELEVATOR" */,					//24
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051E670, "O MOVESEAT" } /* "O MOVESEAT" */,					//25
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051E430, "O EGGMANBED" } /* "O EGGMANBED" */,						//26
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051E320, "O POOL DOOR" } /* "O POOL DOOR" */,					//27
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051E180, "O PROPELLER" } /* "O PROPELLER" */,					//28
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x00524370, "O EC DOOR" } /* "O EC DOOR" */,					//29
	{ 6, 3, 0, 0, 0, (ObjectFuncPtr)0x0051DFA0, "O SKYDECK" } /* "O SKYDECK" */,					//2A
	{ 6, 3, 2, 0, 0, (ObjectFuncPtr)0x0051DC30, "O POOLWATER" } /* "O POOLWATER" */,					//2B
	{ 6, 3, 0, 0, 0, (ObjectFuncPtr)0x0051DAB0, "O PALMBASE" } /* "O PALMBASE" */,					//2C
	{ 6, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D930, "O BCHAIR" } /* "O BCHAIR" */,					//2D
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D840, "O PARASOL" } /* "O PARASOL" */,					//2E
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D760, "O PALM" } /* "O PALM" */,					//2F
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D400, "O PIER" } /* "O PIER" */,					//30
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D030, "O BOOK1" } /* "O BOOK1" */,					//31
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D050, "O BOOK2" } /* "O BOOK2" */,					//32
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051D070, "O BOOK3" } /* "O BOOK3" */,					//33
	{ 6, 3, 2, 0, 0, (ObjectFuncPtr)0x0051CD20, "O MAST" } /* "O MAST" */,					//34
	{ 3, 3, 0, 0, 0, (ObjectFuncPtr)0x007AC4F0, "O LINERING V" } /* "O LINERING V" */,					//35
	{ 3, 3, 0, 0, 0, (ObjectFuncPtr)0x007ABE90, "O LINERING" } /* "O LINERING" */,					//36
	{ 2, 2, 0, 0, 0, (ObjectFuncPtr)0x007A1AA0, "O TIKAL" } /* "O TIKAL" */,					//37
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051CC30, "O IKADA" } /* "O IKADA" */,					//38
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x0051C9A0, "O BOAT" } /* "O BOAT" */,					//39
	{ 2, 3, 0, 0, 0, (ObjectFuncPtr)0x004B4940, "O EMBLEM" } /* "O EMBLEM" */						//3A
};

ObjectList EggCarrierOutObjectList = { arraylengthandptrT(EggCarrierOutObjectList_list, int) };

PointerInfo pointers[] = {
	ptrdecl(0x974E98, &EggCarrierOutObjectList),
	ptrdecl(0x974E9C, &EggCarrierOutObjectList),
	ptrdecl(0x974EA0, &EggCarrierOutObjectList),
	ptrdecl(0x974EA4, &EggCarrierOutObjectList),
	ptrdecl(0x974EA8, &EggCarrierOutObjectList),
	ptrdecl(0x974EAC, &EggCarrierOutObjectList),
};


void Init(const char *path, const HelperFunctions &helperFunctions)
{
	WriteCall((void *)0x52C877, GOODModelRestoration);
	WriteCall((void *)0x521F91, LightningDrawChange);
	WriteCall((void *)0x521D91, LightningDrawChange);
	WriteCall((void *)0x521E70, LightningDrawChange);
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, &Init, NULL, 0, NULL, 0, NULL, 0, arrayptrandlength(pointers) };

	__declspec(dllexport) void cdecl Init()
	{
		
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		
	}
}