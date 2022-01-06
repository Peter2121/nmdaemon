#ifndef MEDIADESC_H
#define MEDIADESC_H

#include <stddef.h>
#include <net/if_media.h>

/**********************************************************************
 * A good chunk of this is duplicated from sys/net/if_media.c
 **********************************************************************/

static struct ifmedia_description ifm_type_descriptions[] =
    IFM_TYPE_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_ethernet_descriptions[] =
    IFM_SUBTYPE_ETHERNET_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_ethernet_aliases[] =
    IFM_SUBTYPE_ETHERNET_ALIASES;

static struct ifmedia_description ifm_subtype_ethernet_option_descriptions[] =
    IFM_SUBTYPE_ETHERNET_OPTION_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_ieee80211_descriptions[] =
    IFM_SUBTYPE_IEEE80211_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_ieee80211_aliases[] =
    IFM_SUBTYPE_IEEE80211_ALIASES;

static struct ifmedia_description ifm_subtype_ieee80211_option_descriptions[] =
    IFM_SUBTYPE_IEEE80211_OPTION_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_ieee80211_mode_descriptions[] =
    IFM_SUBTYPE_IEEE80211_MODE_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_ieee80211_mode_aliases[] =
    IFM_SUBTYPE_IEEE80211_MODE_ALIASES;

static struct ifmedia_description ifm_subtype_atm_descriptions[] =
    IFM_SUBTYPE_ATM_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_atm_aliases[] =
    IFM_SUBTYPE_ATM_ALIASES;

static struct ifmedia_description ifm_subtype_atm_option_descriptions[] =
    IFM_SUBTYPE_ATM_OPTION_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_shared_descriptions[] =
    IFM_SUBTYPE_SHARED_DESCRIPTIONS;

static struct ifmedia_description ifm_subtype_shared_aliases[] =
    IFM_SUBTYPE_SHARED_ALIASES;

static struct ifmedia_description ifm_shared_option_descriptions[] =
    IFM_SHARED_OPTION_DESCRIPTIONS;

static struct ifmedia_description ifm_shared_option_aliases[] =
    IFM_SHARED_OPTION_ALIASES;

struct ifmedia_type_to_subtype {
    struct {
        struct ifmedia_description *desc;
        int alias;
    } subtypes[5];
    struct {
        struct ifmedia_description *desc;
        int alias;
    } options[4];
    struct {
        struct ifmedia_description *desc;
        int alias;
    } modes[3];
};

static struct ifmedia_type_to_subtype ifmedia_types_to_subtypes[] = {
    {
        {
            { &ifm_subtype_shared_descriptions[0], 0 },
            { &ifm_subtype_shared_aliases[0], 1 },
            { &ifm_subtype_ethernet_descriptions[0], 0 },
            { &ifm_subtype_ethernet_aliases[0], 1 },
            { NULL, 0 },
        },
        {
            { &ifm_shared_option_descriptions[0], 0 },
            { &ifm_shared_option_aliases[0], 1 },
            { &ifm_subtype_ethernet_option_descriptions[0], 0 },
            { NULL, 0 },
        },
        {
            { NULL, 0 },
        },
    },
    {
        {
            { &ifm_subtype_shared_descriptions[0], 0 },
            { &ifm_subtype_shared_aliases[0], 1 },
            { &ifm_subtype_ieee80211_descriptions[0], 0 },
            { &ifm_subtype_ieee80211_aliases[0], 1 },
            { NULL, 0 },
        },
        {
            { &ifm_shared_option_descriptions[0], 0 },
            { &ifm_shared_option_aliases[0], 1 },
            { &ifm_subtype_ieee80211_option_descriptions[0], 0 },
            { NULL, 0 },
        },
        {
            { &ifm_subtype_ieee80211_mode_descriptions[0], 0 },
            { &ifm_subtype_ieee80211_mode_aliases[0], 0 },
            { NULL, 0 },
        },
    },
    {
        {
            { &ifm_subtype_shared_descriptions[0], 0 },
            { &ifm_subtype_shared_aliases[0], 1 },
            { &ifm_subtype_atm_descriptions[0], 0 },
            { &ifm_subtype_atm_aliases[0], 1 },
            { NULL, 0 },
        },
        {
            { &ifm_shared_option_descriptions[0], 0 },
            { &ifm_shared_option_aliases[0], 1 },
            { &ifm_subtype_atm_option_descriptions[0], 0 },
            { NULL, 0 },
        },
        {
            { NULL, 0 },
        },
    },
};

#endif // MEDIADESC_H
