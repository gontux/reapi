#include "precompiled.h"

/*
* Compares the entity to specified a classname.
* @note This native also checks the validity of an entity.
*
* @return       true/false
*
* native bool:FClassnameIs(const entityIndex, const className[]);
*/
cell AMX_NATIVE_CALL amx_FClassnameIs(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_classname };

	int nEntityIndex = params[arg_index];
	if (nEntityIndex < 0 || nEntityIndex > gpGlobals->maxEntities) {
		return FALSE;
	}

	edict_t *pEdict = edictByIndex(nEntityIndex);
	if (pEdict && FClassnameIs(pEdict, getAmxString(amx, params[arg_classname]))) {
		return TRUE;
	}

	return FALSE;
}

/*
* To get WeaponIdType from grenade entity
*
* @param entity     Grenade entity
*
* @return           return enum's of WeaponIdType
*
* native WeaponIdType:GetGrenadeType(const entityIndex);
*/
cell AMX_NATIVE_CALL amx_GetGrenadeType(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index };

	CHECK_ISENTITY(arg_index);

	edict_t *pEdict = edictByIndex(params[arg_index]);
	if (!pEdict || !FClassnameIs(pEdict, "grenade")) {
		return WEAPON_NONE;
	}

	CGrenade *pGrenade = getPrivate<CGrenade>(pEdict);
	if (!pGrenade) {
		return WEAPON_NONE;
	}

	if (pGrenade->m_bIsC4) {
		return WEAPON_C4;
	}

	if (!pGrenade->m_usEvent) {
		return WEAPON_FLASHBANG;
	}

	static unsigned short usCreateExplosion = 0;
	if (!usCreateExplosion) {
		usCreateExplosion = PRECACHE_EVENT(1, "events/createexplo.sc");
	}

	return (pGrenade->m_usEvent == usCreateExplosion) ? WEAPON_HEGRENADE : WEAPON_SMOKEGRENADE;
}

/*
* Sets the view entity on a client.
* This allows pfnSetView able to hooks.
*
* @param index       Client index
* @param viewEntity  Entity index
*
* native engset_view(const index, const const viewEntity);
*/
cell AMX_NATIVE_CALL amx_engset_view(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_entity };

	CHECK_ISPLAYER(arg_index);

	CBasePlayer *pPlayer = UTIL_PlayerByIndex(params[arg_index]);
	CHECK_CONNECTED(pPlayer, arg_index);

	g_pengfuncsTable->pfnSetView(pPlayer->edict(), edictByIndexAmx(params[arg_entity]));
	return TRUE;
}

/*
* Gets the return index of the current view entity on a client.
*
* @param index       Client index
*
* native get_viewent(const index);
*/
cell AMX_NATIVE_CALL amx_get_viewent(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index };

	CHECK_ISPLAYER(arg_index);

	client_t *pClient = clientOfIndex(params[arg_index]);
	if (unlikely(pClient == nullptr || !(pClient->active | pClient->spawned | pClient->connected)))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: player %i is not connected", __FUNCTION__, params[arg_index]);
		return FALSE;
	}

	return indexOfEdictAmx(pClient->pViewEntity);
}

/*
* Gets value for key in buffer
*
* @param pbuffer    Pointer to buffer
* @param key        Key string
* @param value      Buffer to copy value to
* @param maxlen     Maximum size of the buffer
*
* @return           Number of cells written to buffer
* @error            If the index is not within the range of 1 to MaxClients or
*                   the client is not connected, an error will be thrown.
*
* native get_key_value(const pbuffer, const key[], const value[], const maxlen);
*/
cell AMX_NATIVE_CALL amx_get_key_value(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_buffer, arg_key, arg_value, arg_maxlen };

	char *buffer = reinterpret_cast<char *>(params[arg_buffer]);
	if (!buffer)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: Invalid buffer", __FUNCTION__);
		return FALSE;
	}

	char key[MAX_KV_LEN];
	Q_strlcpy(key, getAmxString(amx, params[arg_key]));

	return g_amxxapi.SetAmxString(amx, params[arg_value], g_engfuncs.pfnInfoKeyValue(buffer, key), params[arg_maxlen]);
}

/*
* Sets value for key in buffer
*
* @param pbuffer    Pointer to buffer
* @param key        Key string
* @param value      Value to set
*
* @noreturn
*
* native set_key_value(const &pbuffer, const key[], const value[]);
*/
cell AMX_NATIVE_CALL amx_set_key_value(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_buffer, arg_key, arg_value };

	char *buffer = reinterpret_cast<char *>(params[arg_buffer]);
	if (!buffer)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: Invalid buffer", __FUNCTION__);
		return FALSE;
	}

	char key[MAX_KV_LEN], value[MAX_KV_LEN];
	Q_strlcpy(key,    getAmxString(amx, params[arg_key]));
	Q_strlcpy(value,  getAmxString(amx, params[arg_value]));

	if (!key[0])
	{
		buffer[0] = '\0';
		return TRUE;
	}

	Info_SetValueForStarKey(buffer, key, value, MAX_INFO_STRING);

	// TODO: this function doesn't let me sets another buffer
	//g_engfuncs.pfnSetKeyValue(buffer, key, value);
	return TRUE;
}

/*
* Gets the position of the bone
*
* @param entity     Entity index
* @param bone       Number of the bone
* @param vecOrigin  Array to store origin in
* @param vecAngles  Array to store angles in
*
* @return           1 on success, 0 otherwise
* @error            If the index is not within the range of 1 to maxEntities or
*                   the entity is not valid, an error will be thrown.
*
* native GetBonePosition(const entity, const bone, Float:vecOrigin[3], Float:vecAngles[3] = {0.0, 0.0, 0.0});
*/
cell AMX_NATIVE_CALL amx_GetBonePosition(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_bone, arg_origin, arg_angles };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	if (FNullEnt(params[arg_index])) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: worldspawn not allowed", __FUNCTION__);
		return FALSE;
	}

	Vector *pVecOrigin = (Vector *)getAmxAddr(amx, params[arg_origin]);
	Vector *pVecAngles = (PARAMS_COUNT == 4) ? (Vector *)getAmxAddr(amx, params[arg_angles]) : nullptr;
	GetBonePosition(pEntity, params[arg_bone], pVecOrigin, pVecAngles);
	return TRUE;
}

/*
* Gets the position of the attachment
*
* @param entity     Entity index
* @param attachment Number of the attachment
* @param vecOrigin  Array to store origin in
* @param vecAngles  Array to store angles in
*
* @return           1 on success, 0 otherwise
* @error            If the index is not within the range of 1 to maxEntities or
*                   the entity is not valid, an error will be thrown.
*
* native GetAttachment(const entity, const attachment, Float:vecOrigin[3], Float:vecAngles[3] = {0.0, 0.0, 0.0});
*/
cell AMX_NATIVE_CALL amx_GetAttachment(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_bone, arg_attachment, arg_angles };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	if (FNullEnt(params[arg_index])) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: worldspawn not allowed", __FUNCTION__);
		return FALSE;
	}

	Vector *pVecOrigin = (Vector *)getAmxAddr(amx, params[arg_attachment]);
	Vector *pVecAngles = (PARAMS_COUNT == 4) ? (Vector *)getAmxAddr(amx, params[arg_angles]) : nullptr;
	GetAttachment(pEntity, params[arg_bone], pVecOrigin, pVecAngles);
	return TRUE;
}

/*
* Sets callback for entity
*
* @param entity     Entity index
* @param callback   The forward to call
* @note Use "" to reset callback
*
* @noreturn
*
* native SetThink(const ent, const callback[]);
*/
cell AMX_NATIVE_CALL amx_SetThink(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_handler };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	int funcid;
	const char *funcname = getAmxString(amx, params[arg_handler]);
	if (unlikely(funcname == nullptr || funcname[0] == '\0')) {
		pEntity->SetThink(nullptr);
		return TRUE;
	}

	if (unlikely(g_amxxapi.amx_FindPublic(amx, funcname, &funcid) != AMX_ERR_NONE)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: public function \"%s\" not found.", __FUNCTION__, funcname);
		return FALSE;
	}

	return (cell)g_entCallback.SetThink(amx, pEntity, funcname);
}

/*
* Sets callback for entity
*
* @param entity     Entity index
* @param callback   The forward to call
* @note Use "" to reset callback
*
* @noreturn
*
* native SetTouch(const ent, const callback[]);
*/
cell AMX_NATIVE_CALL amx_SetTouch(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_handler };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	int funcid;
	const char *funcname = getAmxString(amx, params[arg_handler]);
	if (unlikely(funcname == nullptr || funcname[0] == '\0')) {
		pEntity->SetTouch(nullptr);
		return TRUE;
	}

	if (unlikely(g_amxxapi.amx_FindPublic(amx, funcname, &funcid) != AMX_ERR_NONE)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: public function \"%s\" not found.", __FUNCTION__, funcname);
		return FALSE;
	}

	return (cell)g_entCallback.SetTouch(amx, pEntity, funcname);
}

/*
* Sets callback for entity
*
* @param entity     Entity index
* @param callback   The forward to call
* @note Use "" to reset callback
*
* @noreturn
*
* native SetUse(const ent, const callback[]);
*/
cell AMX_NATIVE_CALL amx_SetUse(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_handler };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	int funcid;
	const char *funcname = getAmxString(amx, params[arg_handler]);
	if (unlikely(funcname == nullptr || funcname[0] == '\0')) {
		pEntity->SetUse(nullptr);
		return TRUE;
	}

	if (unlikely(g_amxxapi.amx_FindPublic(amx, funcname, &funcid) != AMX_ERR_NONE)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: public function \"%s\" not found.", __FUNCTION__, funcname);
		return FALSE;
	}

	return (cell)g_entCallback.SetUse(amx, pEntity, funcname);
}

/*
* Sets callback for entity
*
* @param entity     Entity index
* @param callback   The forward to call
* @note Use "" to reset callback
*
* @noreturn
*
* native SetBlocked(const ent, const callback[]);
*/
cell AMX_NATIVE_CALL amx_SetBlocked(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_handler };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	int funcid;
	const char *funcname = getAmxString(amx, params[arg_handler]);
	if (unlikely(funcname == nullptr || funcname[0] == '\0')) {
		pEntity->SetBlocked(nullptr);
		return TRUE;
	}

	if (unlikely(g_amxxapi.amx_FindPublic(amx, funcname, &funcid) != AMX_ERR_NONE)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: public function \"%s\" not found.", __FUNCTION__, funcname);
		return FALSE;
	}

	return (cell)g_entCallback.SetBlocked(amx, pEntity, funcname);
}

/*
* Sets callback for entity
* @note Entity should be inherited from CBaseToggle, otherwise server can crash
*
* @param entity     Entity index
* @param callback   The forward to call
* @note Use "" to reset callback
*
* @noreturn
*
* native SetMoveDone(const ent, const callback[]);
*/
cell AMX_NATIVE_CALL amx_SetMoveDone(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index, arg_handler };

	CHECK_ISENTITY(arg_index);

	CBaseEntity *pEntity = getPrivate<CBaseEntity>(params[arg_index]);
	if (unlikely(pEntity == nullptr)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: invalid or uninitialized entity", __FUNCTION__);
		return FALSE;
	}

	int funcid;
	const char *funcname = getAmxString(amx, params[arg_handler]);
	if (unlikely(funcname == nullptr || funcname[0] == '\0')) {
		((CBaseToggle *)pEntity)->SetMoveDone(nullptr);
		return TRUE;
	}

	if (unlikely(g_amxxapi.amx_FindPublic(amx, funcname, &funcid) != AMX_ERR_NONE)) {
		MF_LogError(amx, AMX_ERR_NATIVE, "%s: public function \"%s\" not found.", __FUNCTION__, funcname);
		return FALSE;
	}

	return (cell)g_entCallback.SetMoveDone(amx, pEntity, funcname);
}

AMX_NATIVE_INFO Natives_Common[] =
{
	{ "FClassnameIs",    amx_FClassnameIs    },
	{ "GetGrenadeType",  amx_GetGrenadeType  },
	{ "engset_view",     amx_engset_view     },
	{ "get_viewent",     amx_get_viewent     },
	{ "get_key_value",   amx_get_key_value   },
	{ "set_key_value",   amx_set_key_value   },
	{ "GetBonePosition", amx_GetBonePosition },
	{ "GetAttachment",   amx_GetAttachment   },
	{ "SetThink",        amx_SetThink        },
	{ "SetTouch",        amx_SetTouch        },
	{ "SetUse",          amx_SetUse          },
	{ "SetBlocked",      amx_SetBlocked      },
	{ "SetMoveDone",     amx_SetMoveDone     },

	{ nullptr, nullptr }
};

/*
* Check if the entity is valid
*
* @return       true/false
*
* native bool:is_entity(const entityIndex);
*/
cell AMX_NATIVE_CALL is_entity(AMX *amx, cell *params)
{
	enum args_e { arg_count, arg_index };

	int nIndex = params[arg_index];
	if (nIndex < 0 || nIndex > gpGlobals->maxEntities) {
		return FALSE;
	}

	auto pEntity = getPrivate<CBaseEntity>(nIndex);
	if (!pEntity) {
		return FALSE;
	}

	// if it is the index of the player
	if (pEntity->IsPlayer() && pEntity->has_disconnected) {
		return FALSE;
	}

	return TRUE;
}

/*
* Check if the rehlds is available
*
* @return       true/false
*
* native bool:is_rehlds();
*/
cell AMX_NATIVE_CALL is_rehlds(AMX *amx, cell *params)
{
	return (cell)api_cfg.hasReHLDS();
}

/*
* Check if the regamedll is available
*
* @return       true/false
*
* native bool:is_regamedll();
*/
cell AMX_NATIVE_CALL is_regamedll(AMX *amx, cell *params)
{
	return (cell)api_cfg.hasReGameDLL();
}

/*
* Check if the reunion is available
*
* @return       true/false
*
* native bool:has_reunion();
*/
cell AMX_NATIVE_CALL has_reunion(AMX *amx, cell *params)
{
	return (cell)api_cfg.hasReunion();
}

/*
* Check if the vtc is available
*
* @return       true/false
*
* native bool:has_vtc();
*/
cell AMX_NATIVE_CALL has_vtc(AMX *amx, cell *params)
{
	return (cell)api_cfg.hasVTC();
}

/*
* Check if the rechecker is available
*
* @return       true/false
*
* native bool:has_rechecker();
*/
cell AMX_NATIVE_CALL has_rechecker(AMX *amx, cell *params)
{
	return (cell)api_cfg.hasRechecker();
}

AMX_NATIVE_INFO Natives_Checks[] =
{
	{ "is_entity",     is_entity     },
	{ "is_rehlds",     is_rehlds     },
	{ "is_regamedll",  is_regamedll  },
	{ "has_reunion",   has_reunion   },
	{ "has_vtc",       has_vtc       },
	{ "has_rechecker", has_rechecker },
};

void RegisterNatives_Common()
{
	g_amxxapi.AddNatives(Natives_Common);
	g_amxxapi.AddNatives(Natives_Checks);
}
