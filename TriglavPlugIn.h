/**
 * @file TriglavPlugIn.h
 * @author 青猫 (AonekoSS)
 * @brief クリスタ用フィルターSDKのラッパー
 */
#pragma once
#include <functional>

#include "TriglavPlugInSDK/TriglavPlugInSDK.h"

namespace TriglavPlugIn {
	// サーバー
	using Server = TriglavPlugInServer;

	// レコード
	using RecordSuite = TriglavPlugInRecordSuite;

	// サービス
	using ServiceSuite = TriglavPlugInServiceSuite;
	using StringService = TriglavPlugInStringService;
	using BitmapService = TriglavPlugInBitmapService;
	using OffscreenService = TriglavPlugInOffscreenService;
	using PropertyService = TriglavPlugInPropertyService;
	using PropertyService2 = TriglavPlugInPropertyService2;

	// オブジェクト
	using HostObject = TriglavPlugInHostObject;
	using StringObject = TriglavPlugInStringObject;
	using BitmapObject = TriglavPlugInBitmapObject;
	using OffscreenObject = TriglavPlugInOffscreenObject;
	using PropertyObject = TriglavPlugInPropertyObject;

	// その他
	using Char = TriglavPlugInChar;
	using UniChar = TriglavPlugInUniChar;
	using Bool = TriglavPlugInBool;
	using Int = TriglavPlugInInt;
	using Double = TriglavPlugInDouble;
	using Point = TriglavPlugInPoint;
	using Size = TriglavPlugInSize;
	using Rect = TriglavPlugInRect;
	using Ptr = TriglavPlugInPtr;

	// 画像ブロック
	struct Block {
		Rect rect;
		Ptr address;
		Int rowBytes;
		Int pixelBytes;
		Int r, g, b;
	};

	// ブロック転送処理
	extern void Transfer(const Block& dst, const Block& src);
	extern void Transfer(const Block& dst, const Block& src, const Block& alpha);
	extern void Transfer(const Block& dst, const Block& src, const Block& alpha, const Block& select);

	/// オブジェクトベース（releaseProcで解放するタイプのやつ用）
	template < class OBJECT, class SERVICE >
	class ObjectBase {
	protected:
		using Object = OBJECT;
		using Service = SERVICE;
		constexpr explicit ObjectBase(Server const* server) noexcept : server_{ server }, object_{} {}
		/// 各コンストラクタから呼ぶ
		/// @param owned 所有権ないアクセスの場合はtrueに（あまり寿命考える必要は無さそうだけど）
		constexpr void reset(Service const& service, Object object, bool owned = true) {
			if (!owned) service.retainProc(object);
			object_.reset(object, [&service](Object obj) { service.releaseProc(obj);});
		}
		constexpr auto server() const noexcept { return server_; }
	private:
		Server const* server_;
		std::shared_ptr< std::remove_pointer_t< Object > > object_;
	public:
		// 表向きはオブジェクトそのものとして振る舞う
		constexpr operator Object() const noexcept { return object_.get(); }
		constexpr explicit operator bool() const noexcept { return object_.use_count(); }
	};

	/// 文字列オブジェクト
	class String : public ObjectBase< StringObject, StringService > {
		std::function<Object(Service const*)> make_;
	public:
		// 生文字列から
		String(std::string const& str) : ObjectBase{ nullptr }, make_{ [str](Service const* service) {
			Object object{};
			service->createWithAsciiStringProc(&object, str.c_str(), str.length());
			return object;
		} } {}
		String(const char* str) : String(std::string(str)) {}

		// ユニコードから
		String(std::wstring const& str) : ObjectBase{ nullptr }, make_{ [str](Service const* service) {
			Object object{};
			service->createWithUnicodeStringProc(&object, reinterpret_cast<const UniChar*>(str.c_str()), str.length());
			return object;
		} } {}
		String(const wchar_t* str) : String(std::wstring(str)) {}

		// ここで実体化する
		Object operator()(Server const* server) {
			auto pservice = server->serviceSuite.stringService;
			auto object = make_(pservice);
			if (object) reset(*pservice, object);
			return object;
		}

	private:
		// こっちのインターフェイスは隠しとく
		constexpr operator Object() const noexcept;
		constexpr explicit operator bool() const noexcept;

	public:
		// オブジェクトから文字列
		static std::string convert(Server const* server, StringObject object) {
			auto service = server->serviceSuite.stringService;
			Int length{};
			Char const* str{};
			service->getLocalCodeLengthProc(&length, object);
			service->getLocalCodeCharsProc(&str, object);
			return (length && str) ? std::string(str, length) : std::string();
		}
	};

	// プロパティオブジェクト
	class Property : public ObjectBase< PropertyObject, PropertyService> {
		const Int inputKind = kTriglavPlugInPropertyInputKindDefault; // デフォルト固定
		const Int valueKind = kTriglavPlugInPropertyValueKindDefault; // デフォルト固定
		enum class Type : Int {
			Void = kTriglavPlugInPropertyValueTypeVoid,
			Boolean = kTriglavPlugInPropertyValueTypeBoolean,
			Enumeration = kTriglavPlugInPropertyValueTypeEnumeration,
			Integer = kTriglavPlugInPropertyValueTypeInteger,
			Decimal = kTriglavPlugInPropertyValueTypeDecimal,
			Point = kTriglavPlugInPropertyValueTypePoint,
			String = kTriglavPlugInPropertyValueTypeString,
		};
		using Service2 = PropertyService2;
		auto service() const noexcept { return server()->serviceSuite.propertyService; }
		auto service2() const noexcept { return server()->serviceSuite.propertyService2; }
	public:
		Property(Server const* server) : ObjectBase{ server } {
			auto pservice = service();
			Object object{};
			pservice->createProc(&object);
			if (object) reset(*pservice, object);
		}

		Property(Server const* server, PropertyObject object) : ObjectBase{ server } {
			auto pservice = service();
			if (object) reset(*pservice, object, false);
		}

		// 選択アイテム
		class EnumerationItem {
			Server const* server_;
			Service2 const& service_;
			Property const& propaty_;
			const int key_;
		public:
			EnumerationItem(Server const* server, Service2 const& service, Property const& propaty, int key) :
				server_{ server }, service_{ service }, propaty_{ propaty }, key_{ key } {}
			// アイテム追加
			auto addValue(int val, String name) const {
				service_.addEnumerationItemProc(propaty_, key_, val, name(server_), 0);
			}
		};

		// アイテム追加：真偽
		auto addBooleanItem(int key, String name, bool def) const {
			auto pservice = service();
			pservice->addItemProc(*this, key, static_cast<Int>(Type::Boolean), valueKind, inputKind, name(server()), 0);
			pservice->setBooleanDefaultValueProc(*this, key, def);
		}
		// アイテム追加：整数
		auto addIntegerItem(int key, String name, int def, int min, int max) const {
			auto pservice = service();
			pservice->addItemProc(*this, key, static_cast<Int>(Type::Integer), valueKind, inputKind, name(server()), 0);
			pservice->setIntegerDefaultValueProc(*this, key, def);
			pservice->setIntegerMinValueProc(*this, key, min);
			pservice->setIntegerMaxValueProc(*this, key, max);
		}
		// アイテム追加：実数
		auto addDecimalItem(int key, String name, double def, double min, double max) const {
			auto pservice = service();
			pservice->addItemProc(*this, key, static_cast<Int>(Type::Decimal), valueKind, inputKind, name(server()), 0);
			pservice->setDecimalDefaultValueProc(*this, key, def);
			pservice->setDecimalMinValueProc(*this, key, min);
			pservice->setDecimalMaxValueProc(*this, key, max);
		}
		// アイテム追加：選択
		auto addEnumerationItem(int key, String name) const {
			service()->addItemProc(*this, key, static_cast<Int>(Type::Enumeration), valueKind, inputKind, name(server()), 0);
			return EnumerationItem(server(), *service2(), *this, key);
		}
		// アイテム追加：文字列
		auto addStringItem(int key, String name, int size) const {
			service()->addItemProc(*this, key, static_cast<Int>(Type::String), valueKind, inputKind, name(server()), 0);
			auto pservice = service2();
			pservice->setStringMaxLengthProc(*this, key, size);
		}

		auto setBoolean(int key, bool val) const { service()->setBooleanValueProc(*this, key, val); }
		auto setInteger(int key, int val) const { service()->setIntegerValueProc(*this, key, val); }
		auto setDecimal(int key, double val) const { service()->setDecimalValueProc(*this, key, val); }
		auto setEnumeration(int key, int val) const { service2()->setEnumerationValueProc(*this, key, val); }
		auto setString(int key, String val) const { service2()->setStringValueProc(*this, key, val(server())); }

		int getEnumeration(int key) const { Int val; service2()->getEnumerationValueProc(&val, *this, key); return val; }

		// プロパティ同期
		template <class VAL, class RES>
		inline static bool syncVal(VAL& val, const RES& res) {
			if (val != res) { val = static_cast<VAL>(res); return true; }
			return false;
		}
		bool sync(int key, bool& val) const { Bool res; service()->getBooleanValueProc(&res, *this, key); return syncVal(val, res!=0); }
		bool sync(int key, int& val) const { Int res; service()->getIntegerValueProc(&res, *this, key); return syncVal(val, res); }
		bool sync(int key, float& val) const { Double res; service()->getDecimalValueProc(&res, *this, key); return syncVal(val, res); }
		bool sync(int key, std::string& val) const {
			StringObject obj;
			service2()->getStringValueProc(&obj, *this, key);
			auto res = String::convert(server(), obj);
			return syncVal(val, res);
		}
	};

	// オフスクリーンオブジェクト
	class Offscreen : public ObjectBase< OffscreenObject, OffscreenService > {
		Service const* service_;
		RecordSuite const* record_;
	public:
		Offscreen(Server const* server) : ObjectBase{ server }, service_{ server->serviceSuite.offscreenService }, record_{ &server->recordSuite } {}
		void GetSource() {
			Object object;
			TriglavPlugInFilterRunGetSourceOffscreen(record_, &object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		void GetDestination() {
			Object object;
			TriglavPlugInFilterRunGetDestinationOffscreen(record_, &object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		void GetSelectArea() {
			Object object;
			TriglavPlugInFilterRunGetSelectAreaOffscreen(record_, &object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		std::vector<Rect> GetBlockRects(const Rect& rect) {
			Int count = 0;
			service_->getBlockRectCountProc(&count, *this, const_cast<Rect*>(&rect));
			std::vector<Rect> blockRects(count);
			for (int i = 0; i < count; ++i) { service_->getBlockRectProc(&blockRects[i], i, *this, const_cast<Rect*>(&rect)); }
			return blockRects;
		}
		void GetBlockImage(const Rect& rect, Block& block) {
			Point point = { rect.left, rect.top };
			block.rect = rect;
			service_->getBlockImageProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
			service_->getRGBChannelIndexProc(&block.r, &block.g, &block.b, *this);
		}
		void GetBlockAlpha(const Rect& rect, Block& block) {
			Point point = { rect.left, rect.top };
			block.rect = rect;
			service_->getBlockAlphaProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
		}
		void GetBlockSelectArea(const Rect& rect, Block& block) {
			Point point = { rect.left, rect.top };
			block.rect = rect;
			service_->getBlockSelectAreaProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
		}
	};


	// レコードベース
	// @note 各セレクタ内でしか使っちゃダメな処理群
	class RecordBase {
	protected:
		Server const* server_;
		RecordSuite const* record_;
		HostObject const& hostObject_;
	public:
		RecordBase(Server const* server) :
			server_{ server },
			record_{ &server->recordSuite },
			hostObject_{ server->hostObject } {}
	};

	// モジュール初期化レコード
	class ModuleInitialize : RecordBase {
		TriglavPlugInModuleInitializeRecord* initialize_;
	public:
		ModuleInitialize(Server const* server) : RecordBase{ server }, initialize_{ server->recordSuite.moduleInitializeRecord } {}
		bool Initialize(const std::string& moduleID) {
			// バージョンチェック
			TriglavPlugInInt hostVersion;
			initialize_->getHostVersionProc(&hostVersion, hostObject_);
			if (hostVersion < kTriglavPlugInNeedHostVersion) return false;

			// モジュールIDと種別の設定（フィルターに固定）
			initialize_->setModuleIDProc(hostObject_, String(moduleID)(server_));
			initialize_->setModuleKindProc(hostObject_, kTriglavPlugInModuleSwitchKindFilter);
			return true;
		}
	};

	// フィルタ初期化レコード
	class Initialize : RecordBase {
	public:
		Initialize(Server const* server) : RecordBase{ server } {
			TriglavPlugInGetFilterInitializeRecord(record_);
		}
		void SetCategoryName(String name, char accessKey) {
			TriglavPlugInFilterInitializeSetFilterCategoryName(record_, hostObject_, name(server_), accessKey);
		}
		void SetFilterName(String name, char accessKey) {
			TriglavPlugInFilterInitializeSetFilterName(record_, hostObject_, name(server_), accessKey);
		}
		void SetCanPreview(bool preview) {
			TriglavPlugInFilterInitializeSetCanPreview(record_, hostObject_, preview);
		}
		void SetUseBlankImage(bool blank) {
			TriglavPlugInFilterInitializeSetUseBlankImage(record_, hostObject_, blank);
		}

		enum class Target : Int {
			GrayAlpha = kTriglavPlugInFilterTargetKindRasterLayerGrayAlpha,
			RGBAlpha = kTriglavPlugInFilterTargetKindRasterLayerRGBAlpha,
			CMYKAlpha = kTriglavPlugInFilterTargetKindRasterLayerCMYKAlpha,
			Alpha = kTriglavPlugInFilterTargetKindRasterLayerAlpha,
			BinarizationAlpha = kTriglavPlugInFilterTargetKindRasterLayerBinarizationAlpha,
			BinarizationGrayAlpha = kTriglavPlugInFilterTargetKindRasterLayerBinarizationGrayAlpha,
		};
		void SetTargetKinds(const std::vector<Target>& targets) {
			TriglavPlugInFilterInitializeSetTargetKinds(record_, hostObject_, reinterpret_cast<const Int*>(&targets[0]), targets.size());
		}
		void SetProperty(const Property& property) {
			TriglavPlugInFilterInitializeSetProperty(record_, hostObject_, property);
		}

		using PropertyCallBack = TriglavPlugInPropertyCallBackProc;
		void SetPropertyCallBack(PropertyCallBack callback, Ptr data) {
			TriglavPlugInFilterInitializeSetPropertyCallBack(record_, hostObject_, callback, data);
		}
	};

	// フィルタ実行レコード
	// @note いっぱいあるけど使う奴だけ実装
	class Run : RecordBase {
		Int result_;
	public:
		Run(Server const* server) : RecordBase{ server } {}

		// プロパティ
		PropertyObject GetProperty() const {
			PropertyObject property;
			TriglavPlugInFilterRunGetProperty(record_, &property, hostObject_);
			return property;
		}

		// 選択エリア
		Rect GetSelectArea() const {
			TriglavPlugInRect rect;
			TriglavPlugInFilterRunGetSelectAreaRect(record_, &rect, hostObject_);
			return rect;
		}

		// アップデート
		void UpdateRect(const Rect& rect) const {
			TriglavPlugInFilterRunUpdateDestinationOffscreenRect(record_, hostObject_, &rect);
		}

		// 処理の制御
		enum class States : Int {
			Start = kTriglavPlugInFilterRunProcessStateStart,
			Continue = kTriglavPlugInFilterRunProcessStateContinue,
			End = kTriglavPlugInFilterRunProcessStateEnd,
			Abort = kTriglavPlugInFilterRunProcessStateAbort,
		};
		enum class Results : Int {
			Continue = kTriglavPlugInFilterRunProcessResultContinue,
			Restart = kTriglavPlugInFilterRunProcessResultRestart,
			Exit = kTriglavPlugInFilterRunProcessResultExit,
		};
		// ホストに処理を返す
		Results Process(States states) {
			TriglavPlugInFilterRunProcess(record_, &result_, hostObject_, static_cast<Int>(states));
			return static_cast<Results>(result_);
		}
		// 最後のステート
		Results Result() const { return static_cast<Results>(result_); }

		// 進捗通知
		void Total(int total) { TriglavPlugInFilterRunSetProgressTotal(record_, hostObject_, total); }
		void Progress(int step) { TriglavPlugInFilterRunSetProgressDone(record_, hostObject_, step); }
	};
}
