#pragma once

#include "action.h"

#include "config/app_config_fwd.h"
#include "config/service_config_fwd.h"

#include <magic_enum/magic_enum.hpp>

#include <range/v3/all.hpp>

#include <fmt/format.h>

#include <QString>
#include <QWidget>
#include <QStyle>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QMimeDatabase>
#include <QHash>
#include <QUrl>
#include <QIcon>
#include <QPixmap>
#include <QMimeType>
#include <QKeySequence>
#include <QFileIconProvider>
#include <QNetworkProxy>
#include <QNetworkReply>


namespace views = ranges::views;
namespace actions = ranges::actions;

template< class... Types >
using Variant = std::variant< Types... >;

template< class T >
using Optional = std::optional< T >;

template< class T >
using RefWrap = std::reference_wrapper< T >;

template< class T >
using CRefWrap = std::reference_wrapper< const T >;

template< class T >
using OptionalRef = Optional< RefWrap< T > >;

template< class T >
using OptionalCRef = Optional< CRefWrap< T > >;

enum class OperationStatus
{
    Idle,
    Processing,
    Canceled,
    Failed,
    Succeeded,

    Max
};

struct SettingsView
{
    struct {
        AppConfig::ConstConfigPtr config { nullptr };
    }
    AppSettings;

    struct {
        Optional< QString > rawConfig { std::nullopt };
        ServiceConfig::ConstConfigPtr loadedConfig { nullptr };
    }
    ServicesSettings;

    static SettingsView create(AppConfig::ConstConfigPtr config, QString rawServiceConfig) {
        return SettingsView {
            .AppSettings = { .config = config },
            .ServicesSettings = { .rawConfig = std::move(rawServiceConfig) }
        };
    }

    static SettingsView create(AppConfig::ConstConfigPtr config) {
        return SettingsView {
            .AppSettings = { .config = config }
        };
    }

    static SettingsView create(AppConfig::ConstConfigPtr appConfig, ServiceConfig::ConstConfigPtr serviceConfig) {
        return SettingsView {
            .AppSettings = { .config = appConfig },
            .ServicesSettings = { .loadedConfig = serviceConfig }
        };
    }
};

struct MimeData;
using MimeDataList = QList< MimeData >;

struct MimeData
{
    QMimeType           Type;
    Optional< QPixmap > Icon;
    bool                IsBase64Encoded;
    QString             SourceName;
    QString             SourcePath;
    QByteArray          Data;

    bool isImage() const { return Type.name().startsWith("image"); }
    bool isAudio() const { return Type.name().startsWith("audio"); }
    bool isText()  const { return Type.name().startsWith("text");  }

    static Optional< MimeData > fromFile(const QString& filePath, bool encodeBase64 = true);
    static Optional< MimeData > fromImage(const QImage& pixmap, const QString& filePath, bool encodeBase64 = true);
};

struct ServiceRequestLookUpData
{
    QString ServiceProviderName;
    QString ServiceName;
    QString RequestName;

    bool operator == (const ServiceRequestLookUpData& other) const {
        return ServiceProviderName == other.ServiceProviderName &&
               ServiceName == other.ServiceName &&
               RequestName == other.RequestName;
    }

    QString toString() const {
        return ServiceProviderName + " > " + ServiceName + " > " + RequestName;
    }
};

struct ChatMessage
{
    enum Alignment
    {
        AlignLeft,   // 80% of width to the left
        AlignRight,  // 80% of width to the right
        AlignNone    // 100% of width occupied
    };

    struct Attachment
    {
        QPixmap Image;
        QString Caption;
        bool    IsHidden;
    };

    qint64 Id; // Unique Message ID
    QString Text;
    QColor MessageColor;
    Alignment Alignment;
    QList< Attachment > Attachments;
    QDateTime Timestamp;
};

struct HttpServiceBase
{
    int                         Id;
    QString                     Name;
    QUrl                        Url;
    QString                     Method;
    QHash< QString, QString >   Headers;
    QByteArray                  Body;
};

struct HttpServiceBase2
{
    int                         Id;
    QString                     Name;
    QUrl                        Url;
    QString                     Method;
    QHttpHeaders                Headers;
    QByteArray                  Body;
};

struct HttpServiceRequest : HttpServiceBase
{
    bool                        IgnoreSslErrors;
    bool                        UseProxies;

    struct {
        struct {
            QString Selector;
        } Request;

        struct {
            QString Selector;
        } Reply;
    }                           Display;
};

struct ServiceHotkey
{
    ActionType   Action;
    QKeySequence Shortcut;
    bool         Enabled;
};
using HotkeyList = QList< ServiceHotkey >;

struct ServiceProxy
{
    QString                     Name;
    bool                        IgnoreSslErrors;
    bool                        Enabled;
    QNetworkProxy               NetworkProxy;

    bool operator == (const ServiceProxy&) const = default;
};
using ProxyList = QList< ServiceProxy >;

struct HttpServiceReply : HttpServiceBase2
{
    int                         StatusCode;
};

struct HttpServiceError : HttpServiceBase2
{
    int                         Code;
    QString                     ErrorDescription;
};

struct ServiceReply
{
    int                         Code;
    QString                     MarkdownText;
};

struct ServiceError
{
    int                         Code;
    QString                     ErrorDescription;
};


template< typename T >
struct is_qmap_or_qhash : std::false_type {};

template< typename K, typename V >
struct is_qmap_or_qhash< QHash< K, V > > : std::true_type {};

template< typename K, typename V >
struct is_qmap_or_qhash< QMap< K, V > > : std::true_type {};

template< typename T >
inline constexpr bool is_qmap_or_qhash_v = is_qmap_or_qhash< T >::value;

template< typename T >
struct is_qlist : std::false_type {};

template< typename T >
struct is_qlist< QList< T > > : std::true_type {};

template< typename T >
inline constexpr bool is_qlist_v = is_qlist< T >::value;

template< typename T, typename... Ts >
struct is_one_of : std::disjunction< std::is_same< T, Ts >... > {};

template< typename T, typename... Ts >
inline constexpr bool is_one_of_v = is_one_of< T, Ts... >::value;

template< typename T >
concept is_qdebug_streamable = requires(QDebug dbg, const T& t) {
    dbg << t;
};

// detemines whether type T is natively supported by fmt
template< typename T, typename Char = char >
inline constexpr bool is_fmt_custom_formattable = (fmt::detail::type_constant< T, Char >::value == fmt::detail::type::custom_type)
                                                  && !std::is_fundamental_v< T >
                                                  && !std::is_convertible_v< T, std::string_view >
                                                  && !fmt::detail::is_std_string_like< T >::value;

inline Optional< MimeData > MimeData::fromFile(const QString& filePath, bool encodeBase64)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "MimeData::fromFile: Could not open file for reading:" << filePath;
        return {};
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.size() == 0)
    {
        qWarning() << "MimeData::fromFile: File is empty or does not exist:" << filePath;
        return {};
    }

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension);

    MimeData mimeData;

    mimeData.SourceName = fileInfo.fileName();
    mimeData.SourcePath = filePath;
    mimeData.Type = type;
    mimeData.Data = encodeBase64 ? file.readAll().toBase64() : file.readAll();
    mimeData.IsBase64Encoded = encodeBase64;

    if (mimeData.isImage())
    {
        QPixmap pixmap(filePath);
        if (!pixmap.isNull())
        {
            mimeData.Icon = pixmap;
        }
        else
        {
            qWarning() << "Pixmap from " << filePath << " is null";
        }
    }

    if (!mimeData.Icon)
    {
        auto icon = QFileIconProvider().icon(fileInfo);
        mimeData.Icon = icon.pixmap(64);
    }

    return mimeData;
}

inline Optional<MimeData> MimeData::fromImage(const QImage& image, const QString& filePath, bool encodeBase64)
{
    if (image.isNull())
    {
        qWarning() << "MimeData::fromImage: Input QImage is null";
        return {};
    }

    QMimeDatabase db;

    QFileInfo fileInfo(filePath);
    QString sourceName = fileInfo.fileName();

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
    {
        qWarning() << "MimeDdata::fromImage: Failed to encode QImage as PNG";
        return {};
    }

    MimeData mimeData;

    mimeData.SourceName = sourceName;
    mimeData.SourcePath = filePath;
    mimeData.Type = db.mimeTypeForName("image/png");
    mimeData.Data = encodeBase64 ? imageData.toBase64() : imageData;
    mimeData.IsBase64Encoded = encodeBase64;

    QPixmap pixmap = QPixmap::fromImage(image);
    if (!pixmap.isNull())
    {
        mimeData.Icon = pixmap;
    }
    else
    {
        qWarning() << "MimeData::fromImage: Failed to convert QImage to QPixmap";
        mimeData.Icon = QFileIconProvider().icon(QFileIconProvider::File).pixmap(64);
    }

    return mimeData;
}

inline QString ToQString(std::string_view line)
{
    return QString::fromUtf8(line.data(), line.size());
}

template< class TEnum > requires(std::is_enum_v< TEnum >)
inline QString ToQString(TEnum value)
{
    return ToQString(magic_enum::enum_name(value));
}

template< class T >
inline Optional< T > ToOptional(QVariant value)
{
    return value.canConvert< T >() ? Optional { value.value< T >() } : std::nullopt;
}

template< class To, bool check >
inline Optional< To > QVariantToImpl(QVariant value)
{
    if constexpr (is_qlist_v< To >)
    {
        using ItemType = typename To::value_type;

        if constexpr (check) {
            if (!value.canConvert< QVariantList >()) {
                qWarning() << "Cannot convert to qvariantlist";
                return {};
            }
        }

        QVariantList list = value.toList();
        To result;

        for (const auto& item : list) {
            auto transformed = QVariantToImpl< ItemType, check >(item);

            if constexpr (check) {
                if (!transformed) {
                    qWarning() << "Failed to convert " << item;
                    return {};
                }
            }

            result.append(*transformed);
        }

        return result;
    }
    else if constexpr (is_qmap_or_qhash_v< To >)
    {
        using KeyType = typename To::key_type;
        using ValueType = typename To::mapped_type;

        if constexpr (check) {
            if (!value.canConvert< QVariantMap >()) {
                qWarning() << "Expected map-like value for QHash/QMap conversion";
                return {};
            }
        }

        QVariantMap map = value.value< QVariantMap >();
        To result;

        for (auto it = map.begin(); it != map.end(); ++it)
        {
            auto key = QVariantToImpl< KeyType, check >(it.key());
            auto value = QVariantToImpl< ValueType, check >(it.value());

            if constexpr (check) {
                if (!key) {
                    qWarning() << "Failed to parse key " << it.key();
                    return {};
                }

                if (!value) {
                    qWarning() << "Failed to parse value " << it.value();
                    return {};
                }
            }

            result.emplace(std::move(*key), std::move(*value));
        }

        return result;
    }

    if constexpr (check) {
        if (!value.canConvert< To >()) {
            qWarning() << "Cannot convert " << value;
            return {};
        }
    }

    if constexpr (is_one_of_v< To, int, unsigned int, long long,
                               unsigned long long, float, double >)
    {
        bool ok = true;
        To result;

        if constexpr (std::is_same_v< To, int >)
            result = value.toInt(&ok);
        else if constexpr (std::is_same_v< To, unsigned int >)
            result = value.toUInt(&ok);
        else if constexpr (std::is_same_v< To, long long >)
            result = value.toLongLong(&ok);
        else if constexpr (std::is_same_v< To, unsigned long long >)
            result = value.toULongLong(&ok);
        else if constexpr (std::is_same_v< To, float >)
            result = value.toFloat(&ok);
        else if constexpr (std::is_same_v< To, double >)
            result = value.toDouble(&ok);
        else
            static_assert(!std::is_same_v< To, To >, "Not recognized arithmetic type");

        if constexpr (check) {
            if (!ok) {
                qWarning() << "Failed to convert " << value << " to integer";
            }
        }
    }

    return value.value< To >();
}

template< class To >
inline Optional< To > QVariantToOptional(QVariant value)
{
    return QVariantToImpl< To, true >(value);
}

template< class To >
inline To QVariantTo(QVariant value)
{
    return *QVariantToImpl< To, false >(value);
}

template< class T >
inline QVariant ToQVariant(T&& data)
{
    return QVariant::fromValue(std::forward< T >(data));
}

inline QVariantMap ToQVariant(const MimeData& data)
{
    QVariantMap result;

    result["Type"] = data.Type.name();
    result["IsBase64Encoded"] = data.IsBase64Encoded;
    result["SourceName"] = data.SourceName;
    result["SourcePath"] = data.SourcePath;
    result["Data"] = QString::fromLatin1(data.Data);

    return result;
}

inline QVariantList ToQVariant(const MimeDataList& dataList)
{
    QVariantList result;

    for (const auto& data : dataList) {
        result.push_back(ToQVariant(data));
    }

    return result;
}

inline QVariantMap ToQVariant(const QHttpHeaders& headers)
{
    QVariantMap result;

    // Get all headers as a multi-hash
    const QMultiHash<QByteArray, QByteArray> headerMultiHash = headers.toMultiHash();

    // Group by header name
    QHash<QString, QList<QString>> headerMap;

    for (auto it = headerMultiHash.begin(); it != headerMultiHash.end(); ++it) {
        const QString name = QString::fromUtf8(it.key());
        const QString value = QString::fromUtf8(it.value());

        headerMap[name].append(value);
    }

    // Convert to QVariantHash with appropriate types
    for (auto it = headerMap.begin(); it != headerMap.end(); ++it) {
        if (it.value().size() == 1) {
            result[it.key()] = it.value().first();
        } else {
            result[it.key()] = it.value();
        }
    }

    return result;
}

inline QVariantMap ToQVariant(const HttpServiceRequest& request)
{
    QVariantMap result;

    result["Id"]                = request.Id;
    result["Name"]              = request.Name;
    result["Url"]               = request.Url.toString();
    result["Method"]            = request.Method;
    result["Headers"]           = QVariantHash(request.Headers.begin(), request.Headers.end());
    result["Body"]              = QString::fromUtf8(request.Body);
    result["IgnoreSslErrors"]   = request.IgnoreSslErrors;
    result["UseProxies"]        = request.UseProxies;
    result["Display"]           = QVariantMap({ { "Request", QVariantMap { { "Selector", request.Display.Request.Selector } } },
                                                { "Reply",   QVariantMap { { "Selector", request.Display.Reply.Selector } } } });

    // qDebug() << "Http service request body string from utf8 (delete this log later): " << result["Body"];

    return result;
}

inline QVariantMap ToQVariant(const HttpServiceReply& reply)
{
    QVariantMap result;

    result["Id"]                = reply.Id;
    result["StatusCode"]        = reply.StatusCode;
    result["Name"]              = reply.Name;
    result["Url"]               = reply.Url.toString();
    result["Method"]            = reply.Method;
    result["Headers"]           = ToQVariant(reply.Headers);
    result["Body"]              = QString::fromUtf8(reply.Body);

    // seems like they preserve escaped sequences
    // qDebug() << "Http service reply body byte array (delete this log later): " << result["Body"];
    // qDebug() << "Http service reply body string from utf8 (delete this log later): " << result["Body"];

    return result;
}

inline HttpServiceReply CreateServiceReply(QNetworkReply* reply, int requestId = -1)
{
    HttpServiceReply result;

    result.Id         = requestId;
    result.StatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.Url        = reply->url();
    result.Headers    = reply->headers();
    result.Body       = reply->readAll();

    return result;
}

inline HttpServiceError CreateServiceError(QNetworkReply* reply, int requestId)
{
    HttpServiceError result;

    result.Id               = requestId;
    result.Code             = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.ErrorDescription = reply->errorString();
    result.Url              = reply->url();
    result.Headers          = reply->headers();
    result.Body             = reply->readAll();

    return result;
}

template<>
struct fmt::formatter< QString > : fmt::formatter< fmt::string_view >
{
    template< typename FormatContext >
    auto format(const QString& s, FormatContext& ctx) const {
        QByteArray utf8 = s.toUtf8();
        return fmt::formatter< fmt::string_view >::format(fmt::string_view(utf8.constData(), utf8.size()), ctx);
    }
};

template< class T > requires(is_fmt_custom_formattable< T > && is_qdebug_streamable< T >)
struct fmt::formatter< T > : fmt::formatter< QString >
{
    template< typename FormatContext >
    auto format(const T& s, FormatContext& ctx) const {
        QString result;
        QDebug(&result).nospace().noquote() << s;
        return fmt::formatter< QString >::format(result, ctx);
    }
};

template<>
struct fmt::formatter< OperationStatus > : fmt::formatter< QString > {
    auto format(OperationStatus s, auto& ctx) const {
        return fmt::formatter< QString >::format(ToQString(s), ctx);
    }
};

template < class... TArgs >
inline auto qformat(fmt::format_string< TArgs... > fmt, TArgs&&... args) -> QString
{
    return ToQString(fmt::vformat(fmt.str, fmt::vargs< TArgs... >{ { args... } }));
}

inline QString OperationStatusToColorString(OperationStatus status)
{
    static_assert(static_cast< int >(OperationStatus::Max) == 5, "Change this function if you change the enum");

    switch (status)
    {
        case OperationStatus::Idle:         return "#ffffff";
        case OperationStatus::Processing:   return "#648CC8";
        case OperationStatus::Canceled:     return "#D4A017";
        case OperationStatus::Failed:       return "#BE5050";
        case OperationStatus::Succeeded:    return "#64A064";

        default:
            qCritical() << "Failed to recognize operation status. Falling back to #ffffff. Contact developers.";
            return "#ffffff";
    }
}

inline QDebug operator << (QDebug debug, const ServiceProxy& proxy)
{
    QDebugStateSaver saver(debug);
    return debug.nospace() << "ServiceProxy(Name=" << proxy.Name
                           << ", IgnoreSslErrors=" << proxy.IgnoreSslErrors
                           << ", Enabled: " << proxy.Enabled
                           << ", NetworkProxy=" << proxy.NetworkProxy << ")";
}

inline QDebug operator << (QDebug debug, const ServiceHotkey& hotkey)
{
    QDebugStateSaver saver(debug);
    return debug.nospace() << "ServiceHotkey(Action=" << ToQString(hotkey.Action)
                           << ", Shortcut=" << hotkey.Shortcut.toString()
                           << ")";
}

inline QDebug operator << (QDebug debug, const ServiceRequestLookUpData& data)
{
    QDebugStateSaver saver(debug);
    return debug.nospace() << data.toString();
}

inline Optional< QNetworkProxy > ParseProxyFromEndpoint(const QString& endpoint)
{
    QStringList parts = endpoint.split(':', Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        qWarning() << "Invalid proxy endpoint format (expected 'host:port'):" << endpoint;
        return {};
    }

    bool ok;
    quint16 port = parts[1].toUShort(&ok);
    if (!ok || port == 0) {
        qWarning() << "Invalid port in proxy endpoint:" << endpoint;
        return {};
    }

    QHostAddress address(parts[0]);
    if (address.isNull()) {
        qWarning() << "Invalid host address in proxy endpoint:" << endpoint;
        return {};
    }

    return QNetworkProxy(QNetworkProxy::HttpProxy, address.toString(), port);
}

template< class String = QString >
inline String NetworkProxyToEndpoint(const QNetworkProxy& netProxy);

template<>
inline std::string NetworkProxyToEndpoint< std::string >(const QNetworkProxy& proxy)
{
    return fmt::format("{}:{}", proxy.hostName(), proxy.port());
}

template<>
inline QString NetworkProxyToEndpoint< QString >(const QNetworkProxy& proxy)
{
    return qformat("{}:{}", proxy.hostName(), proxy.port());
}

inline void RefreshStyle(QWidget* widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

inline void SetPropertyAndUpdateUi(QWidget* widget, const char* propertyName, QVariant value, bool updateUi = true)
{
    QVariant oldValue = widget->property(propertyName);
    widget->setProperty(propertyName, value);

    if (updateUi && oldValue != value) {
        RefreshStyle(widget);
    }
}

inline void SetWidgetDirty(QWidget* widget, bool value, bool updateUi = true)
{
    SetPropertyAndUpdateUi(widget, "isDirty", value);
}

inline void SetWidgetValid(QWidget* widget, bool value, bool updateUi = true)
{
    SetPropertyAndUpdateUi(widget, "isValid", value);
}
