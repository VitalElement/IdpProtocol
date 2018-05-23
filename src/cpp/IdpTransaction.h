// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include <stdint.h>

enum class IdpCommandFlags
{
    None,
    ResponseExpected = 0x01
};

enum class IdpResponseCode
{
    OK,
    UnknownCommand,
    InvalidParameters,
    UnknownError,
    NotReady,
    Deferred, //!< when the handler wants to defer the response until later on.
    Internal  //!< Special case where processing needs to occur at packet level.
};
