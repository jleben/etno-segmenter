/*
** Copyright (C) 1998-2010 George Tzanetakis <gtzan@cs.uvic.ca>
** Copyright (C) 2013 Jakob Leben <jakob.leben@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef MARSYAS_BUFFER_HPP
#define MARSYAS_BUFFER_HPP

#include "marsyas/MarSystem.h"

namespace Marsyas
{
/**
    \class Buffer
    \ingroup Processing
    \brief Change the processing block size and overlap

    Buffer accumulates input frames, and passes them on to child MarSystem in blocks
    of arbitrary size, with optional overlap when hopSize is smaller than blockSize.

    Note that hopSize can not be larger than blockSize - it will always be clipped to
    blockSize.

    Controls:
    - \b mrs_natural/blockSize [rw] : amount of samples passed to child MarSystem
    - \b mrs_natural/hopSize [rw] : amount of frames between beginnings of blocks
*/

class marsyas_EXPORT Buffer: public MarSystem
{
private:
    void myUpdate(MarControlPtr sender);
    void addControls();

    MarControlPtr ctrl_blockSize;
    MarControlPtr ctrl_hopSize;

    mrs_natural m_blockSize;
    mrs_natural m_hopSize;

    realvec m_block;
    mrs_natural m_blockIdx;

public:
    Buffer(std::string name);
    Buffer(const Buffer& a);
    ~Buffer();
    MarSystem* clone() const;

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_BUFFER_HPP
