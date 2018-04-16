/*
* Copyright (c) 2013 - 2015, Roland Bock
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <sqlpp11/table.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/column_types.h>
/*
namespace TabFoo_
{
	struct Omega
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "omega";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T omega;
				T& operator()()
				{
					return omega;
				}
				const T& operator()() const
				{
					return omega;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::bigint>;
	};
}

struct TabFoo : sqlpp::table_t<TabFoo, TabFoo_::Omega>
{
	using _value_type = sqlpp::no_value_t;
	struct _alias_t
	{
		static constexpr const char _literal[] = "tab_foo";
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
		template <typename T>
		struct _member_t
		{
			T tabFoo;
			T& operator()()
			{
				return tabFoo;
			}
			const T& operator()() const
			{
				return tabFoo;
			}
		};
	};
};
*/
namespace _Players
{
	struct Id
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "id";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T id;
				T& operator()()
				{
					return id;
				}
				const T& operator()() const
				{
					return id;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::integer, ::sqlpp::tag::must_not_insert, ::sqlpp::tag::must_not_update>;
	};

	struct SteamId
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "steamid";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T steamid;
				T& operator()()
				{
					return steamid;
				}
				const T& operator()() const
				{
					return steamid;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::bigint, ::sqlpp::tag::must_not_insert, ::sqlpp::tag::must_not_update>;
	};

	struct Groups
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "groups";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T groups;
				T& operator()()
				{
					return groups;
				}
				const T& operator()() const
				{
					return groups;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::varchar, ::sqlpp::tag::must_not_update, ::sqlpp::tag::can_be_null>;
	};
}

struct Players : sqlpp::table_t<Players, _Players::Id, _Players::SteamId, _Players::Groups>
{
	using _value_type = sqlpp::no_value_t;
	struct _alias_t
	{
		static constexpr const char _literal[] = "players";
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
		template <typename T>
		struct _member_t
		{
			T players;
			T& operator()()
			{
				return players;
			}
			const T& operator()() const
			{
				return players;
			}
		};
	};
};

namespace _Groups
{
	struct Id
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "id";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T id;
				T& operator()()
				{
					return id;
				}
				const T& operator()() const
				{
					return id;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::integer, ::sqlpp::tag::must_not_insert, ::sqlpp::tag::must_not_update>;
	};

	struct GroupName
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "groupname";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T groupname;
				T& operator()()
				{
					return groupname;
				}
				const T& operator()() const
				{
					return groupname;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::varchar, ::sqlpp::tag::must_not_insert, ::sqlpp::tag::must_not_update>;
	};

	struct Permissions
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "permissions";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T permissions;
				T& operator()()
				{
					return permissions;
				}
				const T& operator()() const
				{
					return permissions;
				}
			};
		};
		using _traits = ::sqlpp::make_traits<::sqlpp::varchar, ::sqlpp::tag::must_not_update, ::sqlpp::tag::can_be_null>;
	};
}

struct Groups : sqlpp::table_t<Groups, _Groups::Id, _Groups::GroupName, _Groups::Permissions>
{
	using _value_type = sqlpp::no_value_t;
	struct _alias_t
	{
		static constexpr const char _literal[] = "groups";
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
		template <typename T>
		struct _member_t
		{
			T groups;
			T& operator()()
			{
				return groups;
			}
			const T& operator()() const
			{
				return groups;
			}
		};
	};
};
/*
namespace TabDateTime_
{
	struct ColDayPoint
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "col_day_point";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T colDayPoint;
				T& operator()()
				{
					return colDayPoint;
				}
				const T& operator()() const
				{
					return colDayPoint;
				}
			};
		};
		using _traits = sqlpp::make_traits<sqlpp::day_point, sqlpp::tag::can_be_null>;
	};
	struct ColTimePoint
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "col_time_point";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template <typename T>
			struct _member_t
			{
				T colTimePoint;
				T& operator()()
				{
					return colTimePoint;
				}
				const T& operator()() const
				{
					return colTimePoint;
				}
			};
		};
		using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
	};
}

struct TabDateTime : sqlpp::table_t<TabDateTime, TabDateTime_::ColDayPoint, TabDateTime_::ColTimePoint>
{
	struct _alias_t
	{
		static constexpr const char _literal[] = "tab_date_time";
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
		template <typename T>
		struct _member_t
		{
			T tabDateTime;
			T& operator()()
			{
				return tabDateTime;
			}
			const T& operator()() const
			{
				return tabDateTime;
			}
		};
	};
};*/