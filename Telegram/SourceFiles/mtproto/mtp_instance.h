/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <map>
#include <set>
#include "mtproto/rpc_sender.h"

namespace MTP {
namespace internal {
class Dcenter;
class Session;
class Connection;
} // namespace internal

class DcOptions;
class AuthKey;
using AuthKeyPtr = std::shared_ptr<AuthKey>;
using AuthKeysList = std::vector<AuthKeyPtr>;

class Instance : public QObject {
	Q_OBJECT

public:
	struct Config {
		static constexpr auto kNoneMainDc = -1;
		static constexpr auto kNotSetMainDc = 0;
		static constexpr auto kDefaultMainDc = 2;
		static constexpr auto kTemporaryMainDc = 1000;

		DcId mainDcId = kNotSetMainDc;
		AuthKeysList keys;
	};
	enum class Mode {
		Normal,
		SpecialConfigRequester,
		KeysDestroyer,
	};
	Instance(not_null<DcOptions*> options, Mode mode, Config &&config);

	Instance(const Instance &other) = delete;
	Instance &operator=(const Instance &other) = delete;

	void resolveProxyDomain(const QString &host);
	void setGoodProxyDomain(const QString &host, const QString &ip);
	void suggestMainDcId(DcId mainDcId);
	void setMainDcId(DcId mainDcId);
	DcId mainDcId() const;
	QString systemLangCode() const;
	QString cloudLangCode() const;

	void setKeyForWrite(DcId dcId, const AuthKeyPtr &key);
	AuthKeysList getKeysForWrite() const;
	void addKeysForDestroy(AuthKeysList &&keys);

	not_null<DcOptions*> dcOptions();

	template <typename TRequest>
	mtpRequestId send(
			const TRequest &request,
			RPCResponseHandler &&callbacks = {},
			ShiftedDcId dcId = 0,
			TimeMs msCanWait = 0,
			mtpRequestId after = 0) {
		return send(
			mtpRequestData::serialize(request),
			std::move(callbacks),
			dcId,
			msCanWait,
			after);
	}

	template <typename TRequest>
	mtpRequestId send(
			const TRequest &request,
			RPCDoneHandlerPtr &&onDone,
			RPCFailHandlerPtr &&onFail = nullptr,
			ShiftedDcId dc = 0,
			TimeMs msCanWait = 0,
			mtpRequestId after = 0) {
		return send(
			request,
			RPCResponseHandler(std::move(onDone), std::move(onFail)),
			dc,
			msCanWait,
			after);
	}

	void sendAnything(ShiftedDcId dcId = 0, TimeMs msCanWait = 0);

	void restart();
	void restart(ShiftedDcId shiftedDcId);
	int32 dcstate(ShiftedDcId shiftedDcId = 0);
	QString dctransport(ShiftedDcId shiftedDcId = 0);
	void ping();
	void cancel(mtpRequestId requestId);
	int32 state(mtpRequestId requestId); // < 0 means waiting for such count of ms
	void killSession(ShiftedDcId shiftedDcId);
	void stopSession(ShiftedDcId shiftedDcId);
	void reInitConnection(DcId dcId);
	void logout(RPCDoneHandlerPtr onDone, RPCFailHandlerPtr onFail);

	std::shared_ptr<internal::Dcenter> getDcById(ShiftedDcId shiftedDcId);
	void unpaused();

	void queueQuittingConnection(std::unique_ptr<internal::Connection> &&connection);

	void setUpdatesHandler(RPCDoneHandlerPtr onDone);
	void setGlobalFailHandler(RPCFailHandlerPtr onFail);
	void setStateChangedHandler(base::lambda<void(ShiftedDcId shiftedDcId, int32 state)> handler);
	void setSessionResetHandler(base::lambda<void(ShiftedDcId shiftedDcId)> handler);
	void clearGlobalHandlers();

	void onStateChange(ShiftedDcId dcWithShift, int32 state);
	void onSessionReset(ShiftedDcId dcWithShift);

	void registerRequest(mtpRequestId requestId, ShiftedDcId dcWithShift);
	mtpRequestId storeRequest(
		mtpRequest &request,
		RPCResponseHandler &&callbacks);
	mtpRequest getRequest(mtpRequestId requestId);
	void clearCallbacksDelayed(std::vector<RPCCallbackClear> &&ids);

	void execCallback(mtpRequestId requestId, const mtpPrime *from, const mtpPrime *end);
	bool hasCallbacks(mtpRequestId requestId);
	void globalCallback(const mtpPrime *from, const mtpPrime *end);

	// return true if need to clean request data
	bool rpcErrorOccured(mtpRequestId requestId, const RPCFailHandlerPtr &onFail, const RPCError &err);

	bool isKeysDestroyer() const;
	void scheduleKeyDestroy(ShiftedDcId shiftedDcId);

	void requestConfig();
	void requestConfigIfOld();
	void requestCDNConfig();
	void setUserPhone(const QString &phone);

	~Instance();

public slots:
	void connectionFinished(internal::Connection *connection);

signals:
	void configLoaded();
	void cdnConfigLoaded();
	void keyDestroyed(qint32 shiftedDcId);
	void allKeysDestroyed();
	void proxyDomainResolved(
		QString host,
		QStringList ips,
		qint64 expireAt);

private slots:
	void onKeyDestroyed(qint32 shiftedDcId);

private:
	mtpRequestId send(
		mtpRequest &&request,
		RPCResponseHandler &&callbacks,
		ShiftedDcId dcId,
		TimeMs msCanWait,
		mtpRequestId after);

	class Private;
	const std::unique_ptr<Private> _private;

};

} // namespace MTP
