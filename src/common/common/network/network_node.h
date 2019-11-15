#pragma once

#include "packet.h"
#include <SFML/Network/UdpSocket.hpp>
#include "packet_buffer.h"

struct CommandPackage {
    Packet packet;
    sf::IpAddress address;
    port_t port;
};

template <typename CommandType> class NetworkNode {
  public:
    NetworkNode() = default;
    NetworkNode(port_t port)
    {
        m_socket.bind(port);
    }

    void setNodeId(peer_id_t id)
    {
        m_nodeId = id;
    }

    void setBlocking(bool block)
    {
        m_socket.setBlocking(block);
    }

    CommandPackage createPacket(CommandType command,
                                Packet::Flag flag = Packet::Flag::None)
    {
        return createCommandPacket(
            command, flag,
            flag == Packet::Flag::Reliable ? m_sequenceNumber++ : 0);
    }

    bool send(CommandPackage &package)
    {
        auto &packet = package.packet;
        bool result = m_socket.send(packet.payload, package.address,
                                    package.port) == sf::Socket::Done;
        if (packet.hasFlag(Packet::Flag::Reliable)) {
            // m_packetBuffer.append(std::move(packet), );
        }
        return result;
    }

    void handleAckPacket(sf::Packet &packet)
    {
        u32 sequence;
        peer_id_t id;
        packet >> sequence >> id;
        m_packetBuffer.tryRemove(sequence, id);
    }

    bool recieve(CommandPackage &package)
    {
        auto &packet = package.packet;
        sf::Packet rawPacket;
        if (m_socket.receive(rawPacket, package.address, package.port) ==
            sf::Socket::Done) {
            packet.initFromPacket(rawPacket);
            if (packet.hasFlag(Packet::Flag::Reliable)) {
                auto ack = createPacket(CommandType::Acknowledgment,
                                        Packet::Flag::Reliable);
                ack.packet.payload << packet.sequenceNumber << m_nodeId;
                ack.address = package.address;
                ack.packet = package.port;
                send(ack);
            }
            return true;
        }
        return false;
    }

    void resendPackets()
    {
        if (!m_packetBuffer.isEmpty()) {
            auto &packet = m_packetBuffer.begin();
            if (!packet.clients.empty()) {
                auto itr = packet.clients.begin();
                send(*packet.clients.begin(), packet.packet);
                packet.clients.erase(itr);
            }
        }
    }

  private:
    sf::UdpSocket m_socket;
    PacketBuffer m_packetBuffer;
    peer_id_t m_nodeId;
    u32 m_sequenceNumber = 0;
};

/*
        if (!m_packetBuffer.isEmpty()) {
            auto &packet = m_packetBuffer.begin();
            if (!packet.clients.empty()) {
                auto itr = packet.clients.begin();
                sendToClient(*packet.clients.begin(), packet.packet);
                packet.clients.erase(itr);
            }
        }
        */