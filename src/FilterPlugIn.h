/**
 * @file FilterPlugIn.h
 * @author 青猫 (AonekoSS)
 * @brief フィルタープラグインAPI
 */
#pragma once
#include <functional>

namespace FilterPlugIn {

/// @note アラインメントとか気にするエリア
#pragma pack(push,1)
	extern "C" {
		typedef	unsigned char	Bool;
		typedef	char			Char;
		typedef	unsigned short	UniChar;
		typedef	unsigned char	UInt8;
		typedef	long int		Int;
		typedef	long long int	Int64;
		typedef	float			Float;
		typedef	double			Double;
		typedef	void* Ptr;

		//	座標
		struct Point { Int x, y; };

		//	サイズ
		struct Size { Int width, height; };

		//	矩形
		struct Rect { Int left, top, right, bottom; };

		//	RGBカラー
		struct RGBColor { UInt8 red, green, blue; };

		//	CMYKカラー
		struct CMYKColor { UInt8 cyan, magenta, yellow, keyplate; };

		//	ホスト
		struct _Host;
		typedef	struct _Host* HostObject;

		//	文字列
		struct	_String;
		typedef	struct _String* StringObject;

		//	ビットマップ
		struct _Bitmap;
		typedef	struct _Bitmap* BitmapObject;

		//	オフスクリーン
		struct _Offscreen;
		typedef	struct _Offscreen* OffscreenObject;

		//	プロパティ
		struct	_Property;
		typedef	struct _Property* PropertyObject;

		enum class CallResult : Int {
			Success = 0,
			Failed = -1,
		};

		enum class Selector : Int {
			ModuleInitialize = 0x0101,
			ModuleTerminate = 0x0102,
			FilterInitialize = 0x0201,
			FilterRun = 0x0202,
			FilterTerminate = 0x0203,
		};

		// プロパティコールバック
		enum class PropertyCallBackNotify : Int {
			ValueChanged = 0x11,
			ButtonPushed = 0x21,
			ValueCheck = 0x31,
		};
		enum class PropertyCallBackResult : Int {
			NoModify = 0x01,
			Modify = 0x02,
			Invalid = 0x03,
		};
		typedef void (*PropertyCallBackProc)(PropertyCallBackResult* result, PropertyObject propertyObject, const Int itemKey, const PropertyCallBackNotify notify, Ptr data);

		// モジュール初期化レコード
		struct ModuleInitializeRecord {
			Int(*getHostVersionProc)(Int* hostVersion, HostObject hostObject);
			Int(*setModuleIDProc)(HostObject hostObject, StringObject moduleID);
			Int(*setModuleKindProc)(HostObject hostObject, const Int moduleKind);
		};

		//	フィルタ初期化レコード
		struct FilterInitializeRecord {
			Int(*setFilterCategoryNameProc)(HostObject hostObject, StringObject filterCategoryName, const Char accessKey);
			Int(*setFilterNameProc)(HostObject hostObject, StringObject filterName, const Char accessKey);
			Int(*setCanPreviewProc)(HostObject hostObject, const Bool canPreview);
			Int(*setUseBlankImageProc)(HostObject hostObject, const Bool useBlankImage);
			Int(*setTargetKindsProc)(HostObject hostObject, const Int* targetKinds, const Int targetKindCount);
			Int(*setPropertyProc)(HostObject hostObject, PropertyObject propertyObject);
			Int(*setPropertyCallBackProc)(HostObject hostObject, PropertyCallBackProc propertyCallBackProc, Ptr data);
		};

		//	フィルタ実行レコード
		struct FilterRunRecord {
			Int(*getPropertyProc)(PropertyObject* propertyObject, HostObject hostObject);
			void* dummy1[5];
			Int(*isAlphaLockedProc)(Bool* locked, HostObject hostObject);
			Int(*getSourceOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*getDestinationOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*getSelectAreaRectProc)(Rect* seclectAreaRect, HostObject hostObject);
			void* dummy2[1];
			Int(*getSelectAreaOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*updateDestinationOffscreenRectProc)(HostObject hostObject, const Rect* updateRect);
			void* dummy3[3];
			Int(*processProc)(Int* result, HostObject hostObject, const Int processState);
			Int(*setProgressTotalProc)(HostObject hostObject, const Int progressTotal);
			Int(*setProgressDoneProc)(HostObject hostObject, const Int progressDone);
		};

		//	文字列サービス
		struct StringService {
			Int(*createWithAsciiStringProc)(StringObject* stringObject, const Char* ascii, const Int length);
			Int(*createWithUnicodeStringProc)(StringObject* stringObject, const UniChar* unicode, const Int length);
			Int(*createWithLocalCodeStringProc)(StringObject* stringObject, const Char* localcode, const Int length);
			Int(*createWithStringIDProc)(StringObject* stringObject, const Int stringID, HostObject hostObject);
			Int(*retainProc)(StringObject stringObject);
			Int(*releaseProc)(StringObject stringObject);
			Int(*getUnicodeCharsProc)(const UniChar** unicode, StringObject stringObject);
			Int(*getUnicodeLengthProc)(Int* length, StringObject stringObject);
			Int(*getLocalCodeCharsProc)(const Char** localcode, StringObject stringObject);
			Int(*getLocalCodeLengthProc)(Int* length, StringObject stringObject);
		};

		//	ビットマップサービス
		struct BitmapService {
			Int(*createProc)(BitmapObject* bitmapObject, const Int width, const Int height, const Int depth, const Int scanline);
			Int(*retainProc)(BitmapObject bitmapObject);
			Int(*releaseProc)(BitmapObject bitmapObject);
			Int(*getWidthProc)(Int* width, BitmapObject bitmapObject);
			Int(*getHeightProc)(Int* height, BitmapObject bitmapObject);
			Int(*getDepthProc)(Int* depth, BitmapObject bitmapObject);
			Int(*getScanlineProc)(Int* scanline, BitmapObject bitmapObject);
			Int(*getAddressProc)(Ptr* address, BitmapObject bitmapObject, const Point* pos);
			Int(*getRowBytesProc)(Int* rowBytes, BitmapObject bitmapObject);
			Int(*getPixelBytesProc)(Int* pixelBytes, BitmapObject bitmapObject);
		};

		//	オフスクリーンサービス
		struct OffscreenService {
			Int(*createPlaneProc)(OffscreenObject* offscreenObject, const Int width, const Int height, const Int depth);
			Int(*retainProc)(OffscreenObject offscreenObject);
			Int(*releaseProc)(OffscreenObject offscreenObject);
			Int(*getWidthProc)(Int* width, OffscreenObject offscreenObject);
			Int(*getHeightProc)(Int* height, OffscreenObject offscreenObject);
			Int(*getRectProc)(Rect* rect, OffscreenObject offscreenObject);
			Int(*getExtentRectProc)(Rect* rect, OffscreenObject offscreenObject);
			Int(*getChannelOrderProc)(Int* channelOrder, OffscreenObject offscreenObject);
			Int(*getRGBChannelIndexProc)(Int* redChannelIndex, Int* greenChannelIndex, Int* blueChannelIndex, OffscreenObject offscreenObject);
			Int(*getCMYKChannelIndexProc)(Int* cyanChannelIndex, Int* magentaChannelIndex, Int* yellowChannelIndex, Int* keytoneChannelIndex, OffscreenObject offscreenObject);
			Int(*getBlockRectCountProc)(Int* blockRectCount, OffscreenObject offscreenObject, Rect* bounds);
			Int(*getBlockRectProc)(Rect* blockRect, Int blockIndex, OffscreenObject offscreenObject, Rect* bounds);
			Int(*getBlockImageProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockAlphaProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockSelectAreaProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockPlaneProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getTileWidthProc)(Int* tileWidth, OffscreenObject offscreenObject);
			Int(*getTileHeightProc)(Int* tileHeight, OffscreenObject offscreenObject);
			Int(*getBitmapProc)(BitmapObject bitmapObject, const Point* bitmapPos, OffscreenObject offscreenObject, const Point* offscreenPos, const Int copyWidth, const Int copyHeight, const Int copyMode);
			Int(*setBitmapProc)(OffscreenObject offscreenObject, const Point* offscreenPos, BitmapObject bitmapObject, const Point* bitmapPos, const Int copyWidth, const Int copyHeight, const Int copyMode);
		};

		//	オフスクリーンサービス2
		struct OffscreenService2 {
			Int(*getBitmapNormalAlphaChannelIndexProc)(Int* alphaChannelIndex, OffscreenObject offscreenObject);
		};

		//	プロパティサービス
		struct PropertyService {
			Int(*createProc)(PropertyObject* propertyObject);
			Int(*retainProc)(PropertyObject propertyObject);
			Int(*releaseProc)(PropertyObject propertyObject);
			Int(*addItemProc)(PropertyObject propertyObject, const Int itemKey, const Int valueType, const Int valueKind, const Int inputKind, StringObject caption, const Char accessKey);
			Int(*setBooleanValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool value);
			Int(*getBooleanValueProc)(Bool* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setBooleanDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool defaultValue);
			Int(*getBooleanDefaultValueProc)(Bool* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getIntegerValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Int defaultValue);
			Int(*getIntegerDefaultValueProc)(Int* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Int minValue);
			Int(*getIntegerMinValueProc)(Int* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Int maxValue);
			Int(*getIntegerMaxValueProc)(Int* maxValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalValueProc)(PropertyObject propertyObject, const Int itemKey, const Double value);
			Int(*getDecimalValueProc)(Double* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Double defaultValue);
			Int(*getDecimalDefaultValueProc)(Double* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Double minValue);
			Int(*getDecimalMinValueProc)(Double* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Double maxValue);
			Int(*getDecimalMaxValueProc)(Double* maxValue, PropertyObject propertyObject, const Int itemKey);
		};

		//	プロパティサービス2
		struct PropertyService2 {
			Int(*setItemStoreValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool storeValue);
			Int(*setPointValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* value);
			Int(*getPointValueProc)(Point* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointDefaultValueKindProc)(PropertyObject propertyObject, const Int itemKey, const Int defaultValueKind);
			Int(*getPointDefaultValueKindProc)(Int* defaultValueKind, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* defaultValue);
			Int(*getPointDefaultValueProc)(Point* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMinMaxValueKindProc)(PropertyObject propertyObject, const Int itemKey, const Int minMaxValueKind);
			Int(*getPointMinMaxValueKindProc)(Int* minMaxValueKind, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* minValue);
			Int(*getPointMinValueProc)(Point* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* maxValue);
			Int(*getPointMaxValueProc)(Point* maxValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setEnumerationValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getEnumerationValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setEnumerationDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getEnumerationDefaultValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*addEnumerationItemProc)(PropertyObject propertyObject, const Int itemKey, const Int value, StringObject caption, const Char accessKey);
			Int(*setStringValueProc)(PropertyObject propertyObject, const Int itemKey, StringObject value);
			Int(*getStringValueProc)(StringObject* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setStringDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, StringObject defaultValue);
			Int(*getStringDefaultValueProc)(StringObject* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setStringMaxLengthProc)(PropertyObject propertyObject, const Int itemKey, const Int maxLength);
			Int(*getStringMaxLengthProc)(Int* maxLength, PropertyObject propertyObject, const Int itemKey);
		};

		// レコードスイート
		struct RecordSuite {
			ModuleInitializeRecord* moduleInitializeRecord;
			void* dumy1[4];
			FilterInitializeRecord* filterInitializeRecord;
			FilterRunRecord* filterRunRecord;
			void* dumy2[256 - 7];
		};

		//	サービススイート
		struct ServiceSuite {
			StringService* stringService;
			BitmapService* bitmapService;
			OffscreenService* offscreenService;
			void* dumy1[1];
			PropertyService* propertyService;
			void* dumy2[3];
			OffscreenService2* offscreenService2;
			PropertyService2* propertyService2;
			void* dumy3[256 - 10];
		};

		// サーバー
		struct Server {
			RecordSuite recordSuite;
			ServiceSuite serviceSuite;
			HostObject hostObject;
		};
	}
#pragma pack(pop)
/// ここからは自由

	// 画像ブロック
	struct Block {
		Rect rect;
		Ptr address;
		Int rowBytes;
		Int pixelBytes;
		Int r, g, b;
		bool needOffset;
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
		const Int inputKind = 0x11;
		const Int valueKind = 0x11;
		enum class Type : Int {
			Void = 0x00,
			Boolean = 0x01,
			Enumeration = 0x02,
			Integer = 0x11,
			Decimal = 0x12,
			Point = 0x21,
			String = 0x31,
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
		auto setStringDefault(int key, String val) const { service2()->setStringDefaultValueProc(*this, key, val(server())); }

		int getEnumeration(int key) const { Int val; service2()->getEnumerationValueProc(&val, *this, key); return val; }

		// プロパティ同期
		template <class VAL, class RES>
		inline static bool syncVal(VAL& val, const RES& res) {
			if (val != res) { val = static_cast<VAL>(res); return true; }
			return false;
		}
		bool sync(int key, bool& val) const { Bool res; service()->getBooleanValueProc(&res, *this, key); return syncVal(val, res != 0); }
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
			record_->filterRunRecord->getSourceOffscreenProc(&object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		void GetDestination() {
			Object object;
			record_->filterRunRecord->getDestinationOffscreenProc(&object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		void GetSelectArea() {
			Object object;
			record_->filterRunRecord->getSelectAreaOffscreenProc(&object, server()->hostObject);
			if (object) reset(*service_, object, false);
		}
		std::vector<Rect> GetBlockRects(const Rect& rect) {
			Int count = 0;
			service_->getBlockRectCountProc(&count, *this, const_cast<Rect*>(&rect));
			std::vector<Rect> blockRects(count);
			for (int i = 0; i < count; ++i) { service_->getBlockRectProc(&blockRects[i], i, *this, const_cast<Rect*>(&rect)); }
			return blockRects;
		}
		Block GetBlockImage(const Rect& rect) {
			Point point = { rect.left, rect.top };
			Block block{};
			service_->getBlockImageProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
			service_->getRGBChannelIndexProc(&block.r, &block.g, &block.b, *this);
			block.rect = rect;
			block.needOffset = false;
			return block;
		}
		Block GetBlockAlpha(const Rect& rect) {
			Point point = { rect.left, rect.top };
			Block block{};
			service_->getBlockAlphaProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
			block.rect = rect;
			block.needOffset = false;
			return block;
		}
		Block GetBlockSelectArea(const Rect& rect) {
			Point point = { rect.left, rect.top };
			Block block{};
			service_->getBlockSelectAreaProc(&block.address, &block.rowBytes, &block.pixelBytes, &block.rect, *this, &point);
			block.rect = rect;
			block.needOffset = false;
			return block;
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
		ModuleInitializeRecord* initialize_;
	public:
		ModuleInitialize(Server const* server) : RecordBase{ server }, initialize_{ server->recordSuite.moduleInitializeRecord } {}
		bool Initialize(const std::string& moduleID) {
			// バージョンチェック
			Int hostVersion;
			initialize_->getHostVersionProc(&hostVersion, hostObject_);
			if (hostVersion < 1) return false;

			// モジュールIDと種別の設定（フィルターに固定）
			initialize_->setModuleIDProc(hostObject_, String(moduleID)(server_));
			initialize_->setModuleKindProc(hostObject_, 0x4380);
			return true;
		}
	};

	// フィルタ初期化レコード
	class Initialize : RecordBase {
	public:
		Initialize(Server const* server) : RecordBase{ server } {
			record_->filterInitializeRecord;
		}
		void SetCategoryName(String name, char accessKey) {
			record_->filterInitializeRecord->setFilterCategoryNameProc(hostObject_, name(server_), accessKey);
		}
		void SetFilterName(String name, char accessKey) {
			record_->filterInitializeRecord->setFilterNameProc(hostObject_, name(server_), accessKey);
		}
		void SetCanPreview(bool preview) {
			record_->filterInitializeRecord->setCanPreviewProc(hostObject_, preview);
		}
		void SetUseBlankImage(bool blank) {
			record_->filterInitializeRecord->setUseBlankImageProc(hostObject_, blank);
		}

		enum class Target : Int {
			GrayAlpha = 0x0101,
			RGBAlpha = 0x0102,
			CMYKAlpha = 0x0103,
			Alpha = 0x0104,
			BinarizationAlpha = 0x0105,
			BinarizationGrayAlpha = 0x0106,
		};
		void SetTargetKinds(const std::vector<Target>& targets) {
			record_->filterInitializeRecord->setTargetKindsProc(hostObject_, reinterpret_cast<const Int*>(&targets[0]), targets.size());
		}
		void SetProperty(const Property& property) {
			record_->filterInitializeRecord->setPropertyProc(hostObject_, property);
		}

		using PropertyCallBack = PropertyCallBackProc;
		void SetPropertyCallBack(PropertyCallBack callback, Ptr data) {
			record_->filterInitializeRecord->setPropertyCallBackProc(hostObject_, callback, data);
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
			record_->filterRunRecord->getPropertyProc(&property, hostObject_);
			return property;
		}

		// 選択エリア
		Rect GetSelectArea() const {
			Rect rect;
			record_->filterRunRecord->getSelectAreaRectProc(&rect, hostObject_);
			return rect;
		}

		// アップデート
		void UpdateRect(const Rect& rect) const {
			record_->filterRunRecord->updateDestinationOffscreenRectProc(hostObject_, &rect);
		}

		// 処理の制御
		enum class States : Int {
			Start = 0x0101,
			Continue = 0x0102,
			End = 0x0103,
			Abort = 0x0104,
		};
		enum class Results : Int {
			Continue = 0x0101,
			Restart = 0x0102,
			Exit = 0x0103,
		};
		// ホストに処理を返す
		Results Process(States states) {
			record_->filterRunRecord->processProc(&result_, hostObject_, static_cast<Int>(states));
			return static_cast<Results>(result_);
		}
		// 最後のステート
		Results Result() const { return static_cast<Results>(result_); }

		// 進捗通知
		void Total(int total) { record_->filterRunRecord->setProgressTotalProc(hostObject_, total); }
		void Progress(int step) { record_->filterRunRecord->setProgressDoneProc(hostObject_, step); }
	};
}
