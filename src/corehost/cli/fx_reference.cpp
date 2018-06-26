// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "fx_ver.h"
#include "fx_reference.h"
#include "roll_fwd_on_no_candidate_fx_option.h"

bool fx_reference_t::is_forward_compatible(const fx_ver_t& other) const
{
    assert(get_fx_version_number() < other);
    if (get_fx_version_number() >= other)
    {
        return true;
    }

    // Verify major roll forward
    if (get_fx_version_number().get_major() != other.get_major()
        && roll_fwd_on_no_candidate_fx != roll_fwd_on_no_candidate_fx_option::major_or_minor)
    {
        return false;
    }

    // Verify minor roll forward
    if (get_fx_version_number().get_minor() != other.get_minor()
        && roll_fwd_on_no_candidate_fx != roll_fwd_on_no_candidate_fx_option::major_or_minor
        && roll_fwd_on_no_candidate_fx != roll_fwd_on_no_candidate_fx_option::minor)
    {
        return false;
    }

    // Verify patch roll forward
    if (get_fx_version_number().get_patch() != other.get_patch()
        && patch_roll_fwd == false)
    {
        return false;
    }

    // Release cannot roll forward to pre-release
    if (!get_fx_version_number().is_prerelease() && other.is_prerelease())
    {
        return false;
    }

    return true;
}

void fx_reference_t::apply_settings_from(const fx_reference_t& from)
{
    if (from.get_fx_version().length() > 0)
    {
        set_fx_version(from.get_fx_version());
    }

    if (from.has_roll_fwd_on_no_candidate_fx)
    {
        set_roll_fwd_on_no_candidate_fx(*from.get_roll_fwd_on_no_candidate_fx());
    }

    if (from.has_patch_roll_fwd)
    {
        set_patch_roll_fwd(*from.get_patch_roll_fwd());
    }
}

void fx_reference_t::merge_roll_forward_settings_from(const fx_reference_t& from)
{
    if (from.has_roll_fwd_on_no_candidate_fx)
    {
        if (!has_roll_fwd_on_no_candidate_fx ||
            (*from.get_roll_fwd_on_no_candidate_fx()) < (*get_roll_fwd_on_no_candidate_fx()))
        {
            set_roll_fwd_on_no_candidate_fx(*from.get_roll_fwd_on_no_candidate_fx());
        }
        
    }

    if (from.has_patch_roll_fwd)
    {
        if (!has_patch_roll_fwd ||
            *from.get_patch_roll_fwd() == false)
        {
            set_patch_roll_fwd(*from.get_patch_roll_fwd());
        }
    }
}
