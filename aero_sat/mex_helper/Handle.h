//
// Created by Jan_L on 08.06.2026.
//
#pragma once
#include <vector>
#include "MatlabLogger.h"

/**
 * Handle class
 *
 * Maps a handle number to an array index and vice versa.
 * Use this class in tandem with a variable length array. Handle will keep track at which array
 * index which element is. This allows to provide a constant handle identifier to a public interface
 * and change their location (array index) in the array as needed.
 */
class Handles : public MatlabLogger
{
public:
    /**
     * Add a new element to the handle
     *
     * Add a new element to the internal array of handles and assign it the smallest unused handle
     * identifier
     * @return assigned handle identifier
     */
    size_t add();

    /**
     * Get array index of handle
     * @param handle unique identifier of handle
     * @return array index corresponding to the handle identifier
     */
    size_t get_object_id(const size_t handle);

    /**
     * Remove handle
     *
     * Remove the given handle from the internal array of handles and return the array index the handle
     * was located before deletion.
     * @param handle unique identifier of handle
     * @return array index corresponding to the handle identifier before deletion
     */
    size_t remove(const size_t handle);

    /**
     * Get number of currently managed handles
     * @return number of handles
     */
    size_t size();
private:
    std::vector<size_t> handles_;
};

