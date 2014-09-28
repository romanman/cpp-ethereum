/*
 This file is part of cpp-ethereum.
 
 cpp-ethereum is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 cpp-ethereum is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */
/** @file EthereumRPC.cpp
 * @autohr Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumRPC.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

EthereumRPCServer::EthereumRPCServer(NetConnection* _conn, NetServiceFace* _service): NetProtocol(_conn), m_service(static_cast<EthereumRPC*>(_service))
{
	
}

void EthereumRPCServer::receiveMessage(NetMsg const& _msg)
{
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	RLP req(_msg.rlp());
	RLPStream resp;
	NetMsgType result;
	switch (_msg.type())
	{
		case RequestSubmitTransaction:
		{
			Secret s(req[0].toHash<Secret>());
			u256 v(req[1].toInt<u256>());
			Address d(req[2].toHash<Address>());
			bytes const& data(req[3].toBytes());
			u256 g(req[4].toInt<u256>());
			u256 gp(req[5].toInt<u256>());
			m_service->ethereum()->transact(s, v, d, data, g, gp);
			
			result = 1;
			break;
		}
			
		case RequestCreateContract:
		{
			Secret s(req[0].toHash<Secret>());
			u256 e(req[1].toInt<u256>());
			bytes const& data(req[2].toBytes());
			u256 g(req[3].toInt<u256>());
			u256 gp(req[4].toInt<u256>());
			Address a(m_service->ethereum()->transact(s, e, data, g, gp));
			
			result = 1;
			resp.appendList(1);
			resp << a;
			break;
		}

		case RequestRLPInject:
		{
			m_service->ethereum()->inject(req[0].toBytesConstRef());
			
			result = 1;
			break;
		}

		case RequestFlushTransactions:
		{
			m_service->ethereum()->flushTransactions();
			
			result = 1;
			break;
		}

		case RequestCallTransaction:
		{
			Secret s(req[0].toHash<Secret>());
			u256 v(req[1].toInt<u256>());
			Address d(req[2].toHash<Address>());
			bytes const& data(req[3].toBytes());
			u256 g(req[4].toInt<u256>());
			u256 gp(req[5].toInt<u256>());
			bytes b(m_service->ethereum()->call(s, v, d, data, g, gp));

			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}

		case RequestBalanceAt:
		{
			u256 b(m_service->ethereum()->balanceAt(Address(req[0].toHash<Address>()), req[1].toInt()));

			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestCountAt:
		{
			int block = req[1].toInt();
			u256 b(m_service->ethereum()->countAt(Address(req[0].toHash<Address>()), block));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestStateAt:
		{
			u256 b(m_service->ethereum()->stateAt(Address(req[0].toHash<Address>()), req[1].toInt<u256>(), req[2].toInt()));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestCodeAt:
		{
			bytes b(m_service->ethereum()->codeAt(Address(req[0].toHash<Address>()), req[1].toInt()));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestStorageAt:
		{
			std::map<u256, u256> store(m_service->ethereum()->storageAt(Address(req[0].toHash<Address>()), req[1].toInt()));

			result = 1;
			resp.appendList(1);
			resp.appendList(store.size());
			for (auto s: store)
				resp << s;
			
			break;
		}

		case RequestMessages:
			// eth::PastMessages messages(unsigned _watchId) const
			// eth::PastMessages messages(eth::MessageFilter const& _filter) const
			
		// InstallWatch:
			// unsigned installWatch(eth::MessageFilter const& _filter)
			// unsigned installWatch(h256 _filterId)
			
		// UninstallWatch:
			// void uninstallWatch(unsigned _watchId)
		
		// PeekWatch:
			// bool peekWatch(unsigned _watchId) const
		
		// CheckWatch:
			// bool checkWatch(unsigned _watchId)
			
		// Number:
			// unsigned number() const
			
		// Pending:
			// eth::Transactions pending() const
			
		// Diff:
			// eth::StateDiff diff(unsigned _txi, h256/int _block) const
			
		// Addresses:
			// Addresses addresses(int _block) const
			
		// GasLimitRemaining:
			// u256 gasLimitRemaining() const
			
		// SetCoinbase:
			// Should we store separate State for each client?
			// void setAddress(Address _us)
			
		// GetCoinbase:
			// Address address() const
			
		// SetMining: What to do here for apps?
		// start/stop/isMining/miningProgress/config
		// eth::MineProgress miningProgress() const

		default:
			result = 2;
	}
	
	NetMsg response(serviceId(), _msg.sequence(), result, RLP(resp.out()));
	connection()->send(response);
}

EthereumRPCClient::EthereumRPCClient(NetConnection* _conn, void *): NetProtocol(_conn)
{
	_conn->setDataMessageHandler(serviceId(), [=](NetMsg const& _msg)
	{
		receiveMessage(_msg);
	});
}

void EthereumRPCClient::receiveMessage(NetMsg const& _msg)
{
	// client should look for Success,Exception, and promised responses
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	// !! check promises and set value
	
	RLP res(_msg.rlp());
	switch (_msg.type())
	{
		case Success:
			if (auto p = m_promises[_msg.sequence()])
				p->set_value(make_shared<NetMsg>(_msg));
			break;
			
		case 2:
			// exception
			break;
	}
}

bytes EthereumRPCClient::performRequest(NetMsgType _type, RLPStream& _s)
{
	promiseResponse p;
	futureResponse f = p.get_future();

	NetMsg msg(serviceId(), nextDataSequence(), _type, RLP(_s.out()));
	{
		lock_guard<mutex> l(x_promises);
		m_promises.insert(make_pair(msg.sequence(),&p));
	}
	connection()->send(msg);

	auto s = f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));
	{
		lock_guard<mutex> l(x_promises);
		m_promises.erase(msg.sequence());
	}
	if (s != future_status::ready)
		throw RPCRequestTimeout();
	return std::move(f.get()->rlp());
}

void EthereumRPCClient::transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	RLPStream s(6);
	s << _secret << _value << _dest << _data << _gas << _gasPrice;
	performRequest(RequestSubmitTransaction, s);
}

Address EthereumRPCClient::transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
	RLPStream s(5);
	s << _secret << _endowment << _init << _gas << _gasPrice;
	
	return Address(RLP(performRequest(RequestCreateContract, s)).toBytes());
}

void EthereumRPCClient::inject(bytesConstRef _rlp)
{
	RLPStream s(1);
	s.append(_rlp);
	performRequest(RequestRLPInject, s);
}

void EthereumRPCClient::flushTransactions()
{
	performRequest(RequestFlushTransactions);
}

bytes EthereumRPCClient::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	RLPStream s(6);
	s << _secret << _value << _dest << _data << _gas << _gasPrice;
	return RLP(performRequest(RequestCallTransaction, s)).toBytes();
}


u256 EthereumRPCClient::balanceAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestBalanceAt, s);
	return u256(RLP(r)[0].toInt<u256>());
}

u256 EthereumRPCClient::countAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestCountAt, s));
	
	return r.toInt<u256>();
}

u256 EthereumRPCClient::stateAt(Address _a, u256 _l, int _block) const
{
	RLPStream s(3);
	s << _a << _l << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestStateAt, s));
	
	return r.toInt<u256>();
}

bytes EthereumRPCClient::codeAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestCodeAt, s));
	
	return r.toBytes();
}

std::map<u256, u256> EthereumRPCClient::storageAt(Address _a, int _block) const
{
	std::map<u256, u256> store;
	
	RLPStream s(2);
	s << _a << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestStorageAt, s);
	for (auto s: RLP(r))
		store.insert(make_pair( s[0].toInt<u256>(), s[1].toInt<u256>() ));
	
	return store;
}

eth::PastMessages EthereumRPCClient::messages(unsigned _watchId) const
{
	
}

eth::PastMessages EthereumRPCClient::messages(eth::MessageFilter const& _filter) const
{
	
}

unsigned EthereumRPCClient::installWatch(eth::MessageFilter const& _filter)
{
	
}

unsigned EthereumRPCClient::installWatch(h256 _filterId)
{
	
}

void EthereumRPCClient::uninstallWatch(unsigned _watchId)
{
	
}

bool EthereumRPCClient::peekWatch(unsigned _watchId) const
{
	
}

bool EthereumRPCClient::checkWatch(unsigned _watchId)
{
	
}

unsigned EthereumRPCClient::number() const
{
	
}

eth::StateDiff EthereumRPCClient::diff(unsigned _txi, h256 _block) const
{
	
}

eth::StateDiff EthereumRPCClient::diff(unsigned _txi, int _block) const
{
	
}

Addresses EthereumRPCClient::addresses(int _block) const
{
}

u256 EthereumRPCClient::gasLimitRemaining() const
{
	
}

void EthereumRPCClient::setAddress(Address _us)
{
	
}

Address EthereumRPCClient::address() const
{

}


