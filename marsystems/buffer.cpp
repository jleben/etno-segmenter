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

#include "marsyas/common.h"
#include "buffer.h"

#include <cassert>
#include <algorithm>

using std::ostringstream;
using namespace Marsyas;

Buffer::Buffer(mrs_string name) :
    MarSystem("Buffer", name),
    m_blockSize(512),
    m_hopSize(512),
    m_blockIdx(0)
{
    isComposite_ = true;

    /// Add any specific controls needed by this MarSystem.
    addControls();
}

Buffer::Buffer(const Buffer& a) : MarSystem(a)
{
    /// All member MarControlPtr have to be explicitly reassigned in
    /// the copy constructor.
    ctrl_blockSize = getControl("mrs_natural/blockSize");
    ctrl_hopSize = getControl("mrs_natural/hopSize");
}


Buffer::~Buffer()
{
}

MarSystem*
Buffer::clone() const
{
	return new Buffer(*this);
}

void
Buffer::addControls()
{
    /// Add any specific controls needed by this MarSystem.

    addControl("mrs_natural/blockSize", m_blockSize);
    addControl("mrs_natural/hopSize", m_hopSize);

    setControlState("mrs_natural/blockSize", true);
    setControlState("mrs_natural/hopSize", true);
}

void
Buffer::myUpdate(MarControlPtr sender)
{
    m_blockSize = ctrl_blockSize->to<mrs_natural>();
    m_hopSize = ctrl_hopSize->to<mrs_natural>();

    if (m_hopSize > m_blockSize) {
        std::ostringstream warning;
        warning << "Buffer: requested hop size (" << m_hopSize << ")"
                << " larger than block size (" << m_blockSize << ")."
                << " A hop size equal to the block size will be used instead.";
        MRSWARN( warning.str() );
        m_hopSize = m_blockSize;
        ctrl_hopSize->setValue( m_hopSize, NOUPDATE );
    }

    if (marsystemsSize_)
    {
        marsystems_[0]->setControl("mrs_natural/inObservations", inObservations_);
        marsystems_[0]->setControl("mrs_natural/inSamples", m_blockSize);
        marsystems_[0]->setControl("mrs_real/israte", israte_);
        marsystems_[0]->setControl("mrs_string/inObsNames", inObsNames_);
        marsystems_[0]->update();

        setControl("mrs_natural/onObservations",
                   marsystems_[0]->getControl("mrs_natural/onObservations"));
        setControl("mrs_natural/onSamples",
                   marsystems_[0]->getControl("mrs_natural/onSamples"));
    }
    else
    {
        setControl("mrs_natural/onObservations", 0);
        setControl("mrs_natural/onSamples", 0);
    }

    if ( m_block.getRows() != inObservations_ || m_block.getCols() != m_blockSize ) {
        m_block.allocate( inObservations_, m_blockSize );
        m_blockIdx = 0;
    }
}

// dst_first must not be between first and first+count!
static void copy( realvec & src, mrs_natural src_first, mrs_natural count,
                  realvec & dst, mrs_natural dst_first )
{
    assert( (&src != &dst) || (dst_first < src_first || dst_first >= src_first + count) );
    assert( src.getRows() == dst.getRows() );

    mrs_real * srcData = src.getData();
    mrs_real * dstData = dst.getData();
    mrs_natural rowCount = src.getRows();

    memcpy( dstData + dst_first * rowCount,
            srcData + src_first * rowCount,
            count * rowCount * sizeof(mrs_real) );
}

void
Buffer::myProcess(realvec& in, realvec& out)
{
    realvec & block = m_block;
    const mrs_natural nIn = inSamples_;
    const mrs_natural nBlock = m_blockSize;
    const mrs_natural nHop = m_hopSize;
    const mrs_natural nOverlap = m_blockSize - m_hopSize;

    mrs_natural sIn = 0;
    mrs_natural sBlock = m_blockIdx;
    mrs_natural nBlockRemain = nBlock - sBlock;

    while (sIn + nBlockRemain <= nIn)
    {
        copy( in, sIn, nBlockRemain,
              block, sBlock );

        sIn += nBlockRemain;

        if (marsystemsSize_)
            marsystems_[0]->process( block, out );

        if (nOverlap)
        {
            copy ( block, nHop, nOverlap,
                   block, 0 );
            sBlock = nOverlap;
        }
        else
        {
            sBlock = 0;
        }

        nBlockRemain = nBlock - sBlock;
    }

    if (sIn < nIn) {
        mrs_natural nInRemain = nIn - sIn;
        copy( in, sIn, nInRemain,
              block, sBlock );
        sBlock += nInRemain;
    }

    m_blockIdx = sBlock;
}
