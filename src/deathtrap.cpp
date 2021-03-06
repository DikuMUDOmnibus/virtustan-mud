// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include <list>
#include <algorithm>
#include "conf.h"
#include "deathtrap.hpp"
#include "constants.h"
#include "db.h"
#include "spells.h"
#include "utils.h"
#include "handler.h"
#include "comm.h"
#include "house.h"
#include "char.hpp"
#include "corpse.hpp"
#include "room.hpp"
#include "fight.h"

extern int has_boat(CHAR_DATA * ch);
extern void die(CHAR_DATA * ch, CHAR_DATA * killer);
extern void death_cry(CHAR_DATA * ch);
extern void reset_affects(CHAR_DATA *ch);

namespace DeathTrap
{

// ������ ������� ����-�� � ����
std::list<ROOM_DATA*> room_list;

void log_death_trap(CHAR_DATA * ch);
void remove_items(CHAR_DATA *ch);

} // namespace DeathTrap

// * ������������� ������ ��� �������� ���� ��� �������������� ������ � ���
void DeathTrap::load()
{
	// �� ������ �������, ������� ������ ����
	room_list.clear();

	for (int i = FIRST_ROOM; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
			room_list.push_back(world[i]);
}

/**
* ���������� ����� ������� � ��������� �� �����������
* \param room - �������, ���. ���������
*/
void DeathTrap::add(ROOM_DATA* room)
{
	std::list<ROOM_DATA*>::const_iterator it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* �������� ������� �� ������ ����-��
* \param room - �������, ���. �������
*/
void DeathTrap::remove(ROOM_DATA* room)
{
	room_list.remove(room);
}

/// �������� ���������� ��, ��������� ������ 2 ������� � ��������.
/// ��� ������ �������� ��� �������, ����� � ������ ������� ������� ���� ���,
/// � ������ ��� ������� � ���������������� ������ �� ch->next_in_room
/// � ������ ���� ���������� ����� �� ���������� ��������� ��������.
void DeathTrap::activity()
{
	std::vector<CHAR_DATA *> tmp_list;

	for (auto it = room_list.cbegin(); it != room_list.cend(); ++it)
	{
		for (CHAR_DATA *ch = (*it)->people; ch; ch = ch->next_in_room)
		{
			tmp_list.push_back(ch);
		}
		for (auto i = tmp_list.cbegin(); i != tmp_list.cend(); ++i)
		{
			CHAR_DATA *ch = *i;
			if (ch->purged() || IS_NPC(ch))
			{
				continue;
			}
			std::string name = ch->get_name_str();

			Damage dmg(SimpleDmg(TYPE_ROOMDEATH),
				MAX(1, GET_REAL_MAX_HIT(ch) >> 2), FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE);

			if (dmg.process(ch, ch) < 0)
			{
				char buf_[MAX_INPUT_LENGTH];
				snprintf(buf_, sizeof(buf_),
					"Player %s died in slow DT (room %d)",
					name.c_str(), (*it)->number);
				mudlog(buf_, LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
		}
		tmp_list.clear();
	}
}

namespace OneWayPortal
{

// ������ ������������� �������� <���� ���������, ������ ���������>
std::map<ROOM_DATA*, ROOM_DATA*> portal_list;

/**
* ���������� ������� � ������
* \param to_room - ���� �������� �����
* \param from_room - ������ ��������
*/
void add(ROOM_DATA* to_room, ROOM_DATA* from_room)
{
	portal_list[to_room] = from_room;
}

/**
* �������� ������� �� ������
* \param to_room - ���� ��������� �����
*/
void remove(ROOM_DATA* to_room)
{
	std::map<ROOM_DATA*, ROOM_DATA*>::iterator it = portal_list.find(to_room);
	if (it != portal_list.end())
		portal_list.erase(it);
}

/**
* �������� �� ������� ������� � ������
* \param to_room - ���� ��������� �����
* \return ��������� �� �������� �����
*/
ROOM_DATA * get_from_room(ROOM_DATA* to_room)
{
	std::map<ROOM_DATA*, ROOM_DATA*>::const_iterator it = portal_list.find(to_room);
	if (it != portal_list.end())
		return it->second;
	return 0;
}

} // namespace OneWayPortal

// * ����������� � ��������� ���� ������ � �� ��� �������� � �� ����������.
void DeathTrap::log_death_trap(CHAR_DATA * ch)
{
	const char *filename = "../log/death_trap.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}
	write_time(file);
	fprintf(file, "%s hit death trap #%d (%s)\n", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)]->name);
}

/**
* ������ ����� ��� ����� � ��: ������ �� 1 �� 3 ������ ������, �� ������ �����������.
* ����� �������� � 1, ����� ��������� ���� ��� ����, ��� �������� ����.
*/
/*
void DeathTrap::remove_items(CHAR_DATA *ch)
{
	int num = number(1, 3);
	int wear = 0;
	bool purge = false;

	for (int i = 1; i < NUM_WEARS; ++i)
	{
		OBJ_DATA *obj = GET_EQ(ch, i);
		if (obj && GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
			++wear;
	}

	send_to_char("\r\n", ch);
	if (wear > num)
	{
		while (num)
		{
			int rnd_num = number(1, NUM_WEARS - 1);
			OBJ_DATA *obj = GET_EQ(ch, rnd_num);
			if (obj && GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
			{
				send_to_char(ch, "���%s %s �����%s � ���� ������ ����� �� ���������� �������.\r\n",
					GET_OBJ_SUF_7(obj), obj->short_description, GET_OBJ_SUF_6(obj));
				obj_from_char(obj);
				extract_obj(obj);
				purge = true;
				--num;
			}
		}
	}
	else if (wear && wear <= num)
	{
		for (int i = 1; i < NUM_WEARS; ++i)
		{
			OBJ_DATA *obj = GET_EQ(ch, i);
			if (obj && GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
			{
				send_to_char(ch, "���%s %s �����%s � ���� ������ ����� �� ���������� �������.\r\n",
					GET_OBJ_SUF_7(obj), obj->short_description, GET_OBJ_SUF_6(obj));
				obj_from_char(obj);
				extract_obj(obj);
				purge = true;
			}
		}
	}

	if (purge)
		send_to_char("�����������, ��� �� ����� ������� ����� �� ��������� ��������� ����...\r\n", ch);
	else
		send_to_char("���������! ��� ������ ������� �������� �� ����...\r\n", ch);

	send_to_char("\r\n", ch);
}
*/

/*
int DeathTrap::check_death_trap(CHAR_DATA * ch)
{
	if (IN_ROOM(ch) == NOWHERE) return false;

	if ((ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && !IS_IMMORTAL(ch))
		|| (real_sector(IN_ROOM(ch)) == SECT_FLYING && !IS_NPC(ch)
			&& !IS_GOD(ch) && !AFF_FLAGGED(ch, AFF_FLY))
		|| (real_sector(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !IS_NPC(ch)
			&& !IS_GOD(ch) && !has_boat(ch)))
	{
		log_death_trap(ch);
		// �� ����� ������ �������� ���� � �� - ��� ������ �� ������ ���������
		if (RENTABLE(ch))
		{
			die(ch, NULL);
			GET_HIT(ch) = GET_MOVE(ch) = 0;
			return true;
		}

		death_cry(ch);
		// ���� ������ �������� �� ����� ������
		if (IS_NPC(ch))
		{
			OBJ_DATA *corpse = make_corpse(ch);
			if (corpse != NULL)
			{
				obj_from_room(corpse);	// ��� ����, ����� ���������� ��� ����������
				extract_obj(corpse);
			}
			GET_HIT(ch) = GET_MOVE(ch) = 0;
			extract_char(ch, TRUE);
			return true;
		}
		// ������ ������ �� ����� � ������� ����� ������ (�� remove_items) � ������� ��� ����� ����
		change_fighting(ch, true);
		reset_affects(ch);
		MemQ_flush(ch);
		for (int i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_MEM(ch, i) = 0;
		remove_items(ch);

		char_from_room(ch);
		int to_room = real_room(GET_LOADROOM(ch));
		// ������� � ������, ��� ���� ��� ������� ��� ���� ����� ��� ��� ������
		if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
			to_room = Clan::CloseRent(to_room);
		if (to_room == NOWHERE)
		{
			SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
			HELL_DURATION(ch) = time(0) + 6;
			to_room = r_helled_start_room;
		}
		char_to_room(ch, to_room);
		look_at_room(ch, to_room);
		GET_HIT(ch) = GET_MOVE(ch) = 0;
		GET_POS(ch) = POS_STUNNED;
		act("$n ����$g � ����� � ���������� ���������...", FALSE, ch, 0, 0, TO_ROOM);
		return true;
	}
	return false;
}
*/

// * ��������� � ������� ��.
int DeathTrap::check_death_trap(CHAR_DATA * ch)
{
	if (IN_ROOM(ch) != NOWHERE && !PRF_FLAGGED(ch, PRF_CODERINFO))
		if ((ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && !IS_IMMORTAL(ch)) ||
				(real_sector(IN_ROOM(ch)) == SECT_FLYING && !IS_NPC(ch)
				 && !IS_GOD(ch) && !AFF_FLAGGED(ch, AFF_FLY))
				|| (real_sector(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !IS_NPC(ch)
					&& !IS_GOD(ch) && !has_boat(ch))
				/*|| (real_sector(IN_ROOM(ch)) == SECT_UNDERWATER && !IS_NPC(ch) //��� ���� ������������.
				&& !IS_GOD(ch) && !AFF_FLAGGED(ch, AFF_WATERBREATH))*/)
		{
			OBJ_DATA *corpse;
			DeathTrap::log_death_trap(ch);
			sprintf(buf1, "Player %s died in DT (room %d)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)));
			mudlog(buf1, LGH, LVL_IMMORT, SYSLOG, TRUE);
			if (RENTABLE(ch))
			{
				die(ch, NULL);
				GET_HIT(ch) = GET_MOVE(ch) = 0;
				return TRUE;
			}
			death_cry(ch);
//29.11.09 ��� ����� ���������� ����� (�) ��������
			GET_RIP_DT(ch)= GET_RIP_DT(ch)+1;
            GET_RIP_DTTHIS(ch)= GET_RIP_DTTHIS(ch)+1;
//����� ������ (�) ��������
			corpse = make_corpse(ch);
			if (corpse != NULL)
			{
				obj_from_room(corpse);	// ��� ����, ����� ���������� ��� ����������
				extract_obj(corpse);
			}
			GET_HIT(ch) = GET_MOVE(ch) = 0;
			extract_char(ch, TRUE);
			return (TRUE);
		}
	return (FALSE);
}

bool DeathTrap::is_slow_dt(int rnum)
{
	if (ROOM_FLAGGED(rnum, ROOM_SLOWDEATH) || ROOM_FLAGGED(rnum, ROOM_ICEDEATH))
		return true;
	return false;
}

/// �������� ���� �� ����� � �������, ���� �� ������� � ������� room_rnum
/// \return ���� > 0, �� �������� ������,
/// ����� - ���� � tunnel_damage() �� ��������
int calc_tunnel_dmg(CHAR_DATA *ch, int room_rnum)
{
	if (!IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& RENTABLE(ch)
		&& ROOM_FLAGGED(room_rnum, ROOM_TUNNEL))
	{
		return std::max(20, GET_REAL_MAX_HIT(ch) >> 3);
	}
	return 0;
}

/// \return true - ���� ����� ����� ����� ��� ����� � ������
/// �������������� �� ������� ���� �� ������ ������
bool DeathTrap::check_tunnel_death(CHAR_DATA *ch, int room_rnum)
{
	const int dam = calc_tunnel_dmg(ch, room_rnum);
	if (dam > 0 && GET_HIT(ch) <= dam * 2)
	{
		return true;
	}
	return false;
}

/// ����� ����� � �� � ���-����� ��� � 2 ������� (SECS_PER_PLAYER_AFFECT)
/// � ������ �� ����� ����� (char_to_room), ����� �� ��� ����� �������
bool DeathTrap::tunnel_damage(CHAR_DATA *ch)
{
	const int dam = calc_tunnel_dmg(ch, IN_ROOM(ch));
	if (dam > 0)
	{
		const int room_rnum = IN_ROOM(ch);
		const std::string name = ch->get_name_str();
		Damage dmg(SimpleDmg(TYPE_TUNNERLDEATH), dam, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);

		if (dmg.process(ch, ch) < 0)
		{
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"Player %s died in tunnel room (room %d)",
				name.c_str(), GET_ROOM_VNUM(room_rnum));
			mudlog(buf_, NRM, LVL_IMMORT, SYSLOG, TRUE);
			return true;
		}
	}
	return false;
}
