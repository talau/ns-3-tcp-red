/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

// m_payloadSize = size of data from IP packet (without IPheader)

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/node.h"
#include "ipv4-header.h"

NS_LOG_COMPONENT_DEFINE ("Ipv4Header");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ipv4Header);

Ipv4Header::Ipv4Header ()
  : m_calcChecksum (false),
    m_payloadSize (0),
    m_identification (0),
    m_tos (0),
    m_ttl (0),
    m_protocol (0),
    m_flags (0),
    m_fragmentOffset (0),
    m_checksum(0),
    m_goodChecksum (true),
    m_options(0),
    m_useOptions(false)
{}

void 
Ipv4Header::EnableChecksum (void)
{
  m_calcChecksum = true;
}

void 
Ipv4Header::SetPayloadSize (uint16_t size)
{
  m_payloadSize = size;
}
uint16_t 
Ipv4Header::GetPayloadSize (void) const
{
  return m_payloadSize;
}

uint16_t 
Ipv4Header::GetIdentification (void) const
{
  return m_identification;
}
void 
Ipv4Header::SetIdentification (uint16_t identification)
{
  m_identification = identification;
}



void 
Ipv4Header::SetTos (uint8_t tos)
{
  m_tos = tos;
}
uint8_t 
Ipv4Header::GetTos (void) const
{
  return m_tos;
}
void 
Ipv4Header::SetMoreFragments (void)
{
  m_flags |= MORE_FRAGMENTS;
}
void
Ipv4Header::SetLastFragment (void)
{
  m_flags &= ~MORE_FRAGMENTS;
}
bool 
Ipv4Header::IsLastFragment (void) const
{
  return !(m_flags & MORE_FRAGMENTS);
}

void 
Ipv4Header::SetDontFragment (void)
{
  m_flags |= DONT_FRAGMENT;
}
void 
Ipv4Header::SetMayFragment (void)
{
  m_flags &= ~DONT_FRAGMENT;
}
bool 
Ipv4Header::IsDontFragment (void) const
{
  return (m_flags & DONT_FRAGMENT);
}

void 
Ipv4Header::SetFragmentOffset (uint16_t offset)
{
  NS_ASSERT (!(offset & (~0x3fff)));
  m_fragmentOffset = offset;
}
uint16_t 
Ipv4Header::GetFragmentOffset (void) const
{
  NS_ASSERT (!(m_fragmentOffset & (~0x3fff)));
  return m_fragmentOffset;
}

void 
Ipv4Header::SetTtl (uint8_t ttl)
{
  m_ttl = ttl;
}
uint8_t 
Ipv4Header::GetTtl (void) const
{
  return m_ttl;
}
  
uint8_t 
Ipv4Header::GetProtocol (void) const
{
  return m_protocol;
}
void 
Ipv4Header::SetProtocol (uint8_t protocol)
{
  m_protocol = protocol;
}

void 
Ipv4Header::SetSource (Ipv4Address source)
{
  m_source = source;
}
Ipv4Address
Ipv4Header::GetSource (void) const
{
  return m_source;
}

void 
Ipv4Header::SetDestination (Ipv4Address dst)
{
  m_destination = dst;
}
Ipv4Address
Ipv4Header::GetDestination (void) const
{
  return m_destination;
}

void
Ipv4Header::SetOptions (uint32_t opts)
{
  m_options = opts;
}
uint32_t
Ipv4Header::GetOptions (void) const
{
  return m_options;
}
void 
Ipv4Header::EnableIPOptions (void)
{
  m_useOptions = true;
}

bool
Ipv4Header::IsChecksumOk (void) const
{
  return m_goodChecksum;
}

TypeId 
Ipv4Header::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4Header")
    .SetParent<Header> ()
    .AddConstructor<Ipv4Header> ()
    ;
  return tid;
}
TypeId 
Ipv4Header::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void 
Ipv4Header::Print (std::ostream &os) const
{
  // ipv4, right ?
  std::string flags;
  if (m_flags == 0)
    {
      flags = "none";
    }
  else if (m_flags & MORE_FRAGMENTS &&
           m_flags & DONT_FRAGMENT)
    {
      flags = "MF|DF";
    }
  else if (m_flags & DONT_FRAGMENT)
    {
      flags = "DF";
    }
  else if (m_flags & MORE_FRAGMENTS)
    {
      flags = "MF";
    }
  else
    {
      flags = "XX";
    }

  std::string options = "no";

  if (m_useOptions)
    {
      options = "yes";
    }

  os << "tos 0x" << std::hex << m_tos << std::dec << " "
     << "ttl " << m_ttl << " "
     << "id " << m_identification << " "
     << "protocol " << m_protocol << " "
     << "offset " << m_fragmentOffset << " "
     << "flags [" << flags << "] "
    // this need to be fixed if option field is enabled
     << "length: " << (m_payloadSize + 5 * 4) << " "
     << "options: " << options
     << " " 
     << m_source << " > " << m_destination
    ;
}
uint32_t 
Ipv4Header::GetSerializedSize (void) const
{
  return (uint32_t) GetHeaderSize();
}

uint16_t
Ipv4Header::GetHeaderSize (void) const
{
  // 20 bytes if it is without options field
  uint16_t ipsize = 20;

  // you *cannot* use m_useOptions here.
  if (Node::IPOptionsEnabled ())
    {
      // add 4 bytes for 32 bits from options
      ipsize += 4;  
    } 

  return ipsize;
}

void
Ipv4Header::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  
  uint8_t verIhl = (4 << 4) | (5);
  // if use IP option add a 32bits block
  if (m_useOptions)
    {
      verIhl += 1;
    }
  i.WriteU8 (verIhl);
  i.WriteU8 (m_tos);
  // total lenght of IP datagram, remember it include data field, where TCP/other protocols stay
  i.WriteHtonU16 (m_payloadSize + GetHeaderSize ());
  i.WriteHtonU16 (m_identification);
  uint32_t fragmentOffset = m_fragmentOffset / 8;
  uint8_t flagsFrag = (fragmentOffset >> 8) & 0x1f;
  if (m_flags & DONT_FRAGMENT) 
    {
      flagsFrag |= (1<<6);
    }
  if (m_flags & MORE_FRAGMENTS) 
    {
      flagsFrag |= (1<<5);
    }
  i.WriteU8 (flagsFrag);
  uint8_t frag = fragmentOffset & 0xff;
  i.WriteU8 (frag);
  i.WriteU8 (m_ttl);
  i.WriteU8 (m_protocol);
  i.WriteHtonU16 (0);
  i.WriteHtonU32 (m_source.Get ());
  i.WriteHtonU32 (m_destination.Get ());

  if (m_useOptions)
    {
      i.WriteU32 (m_options);
    } 

  if (m_calcChecksum) 
    {
      //      exit (0);
      i = start;
      uint16_t checksum = i.CalculateIpChecksum(GetHeaderSize ());
      NS_LOG_LOGIC ("checksum=" <<checksum);
      i = start;
      i.Next (10);
      i.WriteU16 (checksum);
    }
}
uint32_t
Ipv4Header::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t verIhl = i.ReadU8 ();
  uint8_t ihl = verIhl & 0x0f; 
  // if IP options was used, ihl have the value from them
  uint16_t headerSize = ihl * 4;
  
  NS_ASSERT ((verIhl >> 4) == 4);
  m_tos = i.ReadU8 ();
  uint16_t size = i.ReadNtohU16 ();
  m_payloadSize = size - headerSize;
  m_identification = i.ReadNtohU16 ();
  uint8_t flags = i.ReadU8 ();
  m_flags = 0;
  if (flags & (1<<6)) 
    {
      m_flags |= DONT_FRAGMENT;
    }
  if (flags & (1<<5)) 
    {
      m_flags |= MORE_FRAGMENTS;
    }
  i.Prev ();
  m_fragmentOffset = i.ReadU8 () & 0x1f;
  m_fragmentOffset <<= 8;
  m_fragmentOffset |= i.ReadU8 ();
  m_fragmentOffset <<= 3;
  m_ttl = i.ReadU8 ();
  m_protocol = i.ReadU8 ();
  m_checksum = i.ReadU16();
  /* i.Next (2); // checksum */
  m_source.Set (i.ReadNtohU32 ());
  m_destination.Set (i.ReadNtohU32 ());

  if (m_useOptions)
    {
       m_options = i.ReadU32();
    }

  if (m_calcChecksum) 
    {
      i = start;
      uint16_t checksum = i.CalculateIpChecksum(headerSize);
      NS_LOG_LOGIC ("checksum=" <<checksum);

      m_goodChecksum = (checksum == 0);
    }
  
  return GetSerializedSize ();
}

}; // namespace ns3
