#pragma once

#include <sqlpp11/table.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/column_types.h>

namespace _Players
{
	struct Id
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "Id";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::integer, sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
	};

	struct SteamId
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "SteamId";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::bigint, sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
	};

	struct Groups
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "PermissionGroups";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::must_not_update, sqlpp::tag::can_be_null>;
	};
}

struct Players : sqlpp::table_t<Players, _Players::Id, _Players::SteamId, _Players::Groups>
{
	using _value_type = sqlpp::no_value_t;

	struct _alias_t
	{
		static constexpr const char _literal[] = "Players";
		using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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
			static constexpr const char _literal[] = "Id";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::integer, sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
	};

	struct GroupName
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "GroupName";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
	};

	struct Permissions
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "Permissions";
			using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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

		using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::must_not_update, sqlpp::tag::can_be_null>;
	};
}

struct Groups : sqlpp::table_t<Groups, _Groups::Id, _Groups::GroupName, _Groups::Permissions>
{
	using _value_type = sqlpp::no_value_t;

	struct _alias_t
	{
		static constexpr const char _literal[] = "PermissionGroups";
		using _name_t = sqlpp::make_char_sequence<sizeof _literal, _literal>;

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
