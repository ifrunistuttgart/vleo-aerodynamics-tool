//
// Created by Jan_L on 08.06.2026.
//

#include "Handle.h"

size_t Handles::add()
{
    size_t new_handle = 0;
    for( const size_t handle : handles_ )
    {
        if( handle == new_handle )
        {
            ++new_handle;
        }
        else
        {
            break;
        }
    }

    handles_.push_back(new_handle);
    LOG(LEVEL_INFO, "Add new handle: " + std::to_string(new_handle));
    return new_handle;
}

size_t Handles::get_object_id(const size_t handle)
{
    const size_t n_handles = handles_.size();
    for( size_t i = 0; i < n_handles; ++i )
    {
        if( handle == handles_[i] )
        {
            LOG(LEVEL_INFO, "Handle : " + std::to_string(handle) + " maps to index: " + std::to_string(i));
            return i;
        }
    }
    throw std::out_of_range("Unknown handle: " + std::to_string(handle));
}

size_t Handles::remove(const size_t handle)
{
    const size_t idx = get_object_id(handle);
    handles_.erase(handles_.begin() + idx);
    LOG(LEVEL_INFO, "Erased handle : " + std::to_string(handle));
    return idx;
}

size_t Handles::size()
{
    const size_t n_handles = handles_.size();
    LOG(LEVEL_INFO, "Number of handles : " + std::to_string(n_handles));
    return n_handles;
}

