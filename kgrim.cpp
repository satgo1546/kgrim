#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QStandardPaths>
#include <QVariantMap>

#include <cstdio>

#include <unistd.h>
#include <fcntl.h>

using namespace Qt::StringLiterals;

#ifndef VERSION
#define VERSION "dev"
#endif

[[noreturn]] static void die(const QString &message) {
	QTextStream(stderr) << "kgrim: " << message << '\n';
	std::exit(1);
}

int main(int argc, char *argv[]) {
	QCoreApplication application(argc, argv);
	QCoreApplication::setApplicationName(u"kgrim"_s);
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QImageWriter writer;
	QString filename;
	QString method;
	QVariantList arguments;
	double optionScale = 0;
	{
		QCommandLineParser parser;
		parser.addHelpOption();
		parser.addVersionOption();
		parser.addOptions({
			{ u"g"_s, u"Capture a region in `wxh+x+y` or `x,y wxh` format, or `-` to read the region from stdin."_s, u"geometry"_s },
			{ u"o"_s, u"Capture a screen by its output name (e.g. DP-1), or the active screen if argument is empty."_s, u"output"_s },
			{ u"T"_s, u"Capture a window by its UUID, or the active window if argument is empty."_s, u"identifier"_s },
			{ u"s"_s, u"Set the output image scale factor. (default: native)"_s, u"factor"_s },
			{ u"t"_s, u"Set output file type. (possible values: png, ppm, jpeg) (default: png)"_s, u"type"_s, u"png"_s },
			{ u"q"_s, u"Set JPEG quality (0-100)."_s, u"quality"_s, u"80"_s },
			{ u"l"_s, u"Set PNG compression level (0-9)."_s, u"level"_s, u"6"_s },
			{ u"c"_s, u"Include cursors in the screenshot."_s },
		});
		parser.addPositionalArgument(u"filename"_s, u"Output filename, or `-` for stdout."_s, u"[filename]"_s);
		parser.process(application);

		QString format = parser.value(u"t"_s);
		if (format != "png" && format != "ppm" && format != "jpeg") {
			die(u"-t format invalid"_s);
		}
		writer.setFormat(format.toLatin1());
		if (parser.isSet(u"q"_s)) {
			if (format != "jpeg") {
				die(u"-q quality is for jpeg only"_s);
			}
			bool ok;
			int quality = parser.value(u"q"_s).toInt(&ok);
			if (!ok || quality < 0 || quality > 100) {
				die(u"-q quality must be an integer between 0 and 100"_s);
			}
			writer.setQuality(quality);
		}
		if (parser.isSet(u"l"_s)) {
			if (format != "png") {
				die(u"-l level is for png only"_s);
			}
			bool ok;
			int level = parser.value(u"l"_s).toInt(&ok);
			if (!ok || level < 0 || level > 9) {
				die(u"-l level must be an integer between 0 and 9"_s);
			}
			// https://github.com/qt/qtbase/blob/v6.12.0/src/gui/image/qpnghandler.cpp#L1023
			// compression = (compression * 9) / 91; // map [0,100] -> [0,9]
			writer.setCompression(level * 11);
		}
		if (parser.isSet(u"g"_s) + parser.isSet(u"o"_s) + parser.isSet(u"T"_s) > 1) {
			// -g and -T can be used together in grim, but this usage is not documented.
			die(u"-g, -o, -T are mutually exclusive"_s);
		}
		if (parser.isSet(u"g"_s)) {
			method = u"CaptureArea"_s;
			QString geometry_string = parser.value(u"g"_s);
			if (geometry_string == u"-"_s) {
				geometry_string = QTextStream(stdin).readLine();
			}
			geometry_string = geometry_string.trimmed();
			QRegularExpressionMatch match = QRegularExpression(u"^([+-]?[0-9]+), *([+-]?[0-9]+) +([0-9]+)x([0-9]+)$"_s).match(geometry_string);
			QStringList groups;
			if (match.hasMatch()) {
				groups = match.capturedTexts();
			} else {
				match = QRegularExpression(u"^([0-9]+)x([0-9]+)([+-][0-9]+)([+-][0-9]+)$"_s).match(geometry_string);
				if (match.hasMatch()) {
					groups = { u""_s, match.captured(3), match.captured(4), match.captured(1), match.captured(2) };
				} else {
					die(u"-g geometry invalid"_s);
				}
			}
			bool ok = false;
			int x = groups[1].toInt(&ok);
			if (!ok) {
				die(u"-g geometry invalid x"_s);
			}
			int y = groups[2].toInt(&ok);
			if (!ok) {
				die(u"-g geometry invalid y"_s);
			}
			uint width = groups[3].toUInt(nullptr);
			if (!width) {
				die(u"-g geometry invalid width"_s);
			}
			uint height = groups[4].toUInt(nullptr);
			if (!height) {
				die(u"-g geometry invalid height"_s);
			}
			arguments << x << y << width << height;
		} else if (parser.isSet(u"o"_s)) {
			QString screenName = parser.value(u"o"_s);
			if (screenName.isEmpty()) {
				method = u"CaptureActiveScreen"_s;
			} else {
				method = u"CaptureScreen"_s;
				arguments << screenName;
			}
		} else if (parser.isSet(u"T"_s)) {
			QString windowHandle = parser.value(u"T"_s);
			if (windowHandle.isEmpty()) {
				method = u"CaptureActiveWindow"_s;
			} else {
				method = u"CaptureWindow"_s;
				arguments << windowHandle;
			}
		} else {
			method = u"CaptureWorkspace"_s;
		}
		if (parser.isSet(u"s"_s)) {
			bool ok;
			optionScale = parser.value(u"s"_s).toDouble(&ok);
			if (!ok || !qIsFinite(optionScale) || optionScale <= 0) {
				die(u"-s scale invalid"_s);
			}
		}
		arguments << QVariantMap {
			{ u"include-cursor"_s, parser.isSet(u"c"_s) },
			{ u"native-resolution"_s, optionScale == 1 ? optionScale = 0, false : true },
			{ u"include-decoration"_s, true },
			{ u"include-shadow"_s, true },
			{ u"hide-caller-windows"_s, false },
		};
		const QStringList args = parser.positionalArguments();
		if (args.size() > 1) {
			die(u"too many filenames"_s);
		}
		if (args.isEmpty()) {
			QString directory = qEnvironmentVariable("GRIM_DEFAULT_DIR");
			if (directory.isEmpty() || !QFileInfo(directory).isDir()) {
				directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
				if (directory.isEmpty() || !QFileInfo(directory).isDir()) {
					directory = QDir::currentPath();
				}
			}
			filename = QDir(directory).filePath(QDateTime::currentDateTime().toString(u"yyyyMMdd_HH'h'mm'm'ss's_kgrim.'"_s) + format);
		} else {
			filename = args.first();
		}
	}

	QImage image;
	double capturedScale;
	{
		const QDBusConnection bus = QDBusConnection::sessionBus();
		if (!bus.isConnected()) {
			die(u"failed to connect to the session bus: %1"_s.arg(bus.lastError().message()));
		}
		int pipeFds[2];
		if (::pipe2(pipeFds, O_CLOEXEC) != 0) {
			std::perror("kgrim: failed to create screenshot pipe: ");
			return 1;
		}
		arguments << QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1]));
		QDBusMessage reply;
		{
			QDBusMessage request = QDBusMessage::createMethodCall(
				u"org.kde.KWin.ScreenShot2"_s,
				u"/org/kde/KWin/ScreenShot2"_s,
				u"org.kde.KWin.ScreenShot2"_s,
				method
			);
			request.setArguments(arguments);
			reply = bus.call(request, QDBus::Block);
		}
		::close(pipeFds[1]);
		if (reply.type() == QDBusMessage::ErrorMessage) {
			die(u"%1: %2: %3"_s.arg(method, reply.errorName(), reply.errorMessage()));
		}
		if (reply.arguments().size() != 1) {
			die(u"%1 replied with %2 arguments?!"_s.arg(method, reply.arguments().size()));
		}
		const QVariantMap metadata = qdbus_cast<QVariantMap>(reply.arguments().at(0));
		if (metadata[u"type"_s].toString() != u"raw"_s) {
			die(u"%1 replied with unsupported type %2"_s.arg(method, metadata[u"type"_s].toString()));
		}
		// Conveniently, zero is invalid for all the following fields, so no need for &ok.
		const uint format = metadata[u"format"_s].toUInt(nullptr);
		const int width = metadata[u"width"_s].toInt(nullptr);
		const int height = metadata[u"height"_s].toInt(nullptr);
		const double scale = metadata[u"scale"_s].toDouble(nullptr);
		if (!format || format >= QImage::NImageFormats || width <= 0 || height <= 0) {
			die(u"%1 reply invalid"_s.arg(method));
		}
		image = QImage(width, height, QImage::Format(format));
		capturedScale = scale ? scale : 1;
		QFile file;
		if (!file.open(pipeFds[0], QFileDevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
			::close(pipeFds[0]);
			die(u"failed to open pipe for reading"_s);
		}
		QDataStream stream(&file);
    stream.readRawData(reinterpret_cast<char *>(image.bits()), image.sizeInBytes());
	}
	if (optionScale) {
		image = image.scaled(
			qRound(image.width() / capturedScale * optionScale),
			qRound(image.height() / capturedScale * optionScale),
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation
		);
	}

	QFile file;
	if (filename == u"-"_s) {
		if (!file.open(STDOUT_FILENO, QIODevice::WriteOnly)) {
			die(u"failed to open stdout: %1"_s.arg(file.errorString()));
		}
		writer.setDevice(&file);
	} else {
		writer.setFileName(filename);
	}
	if (!writer.write(image)) {
		die(u"failed to write image: %1"_s.arg(writer.errorString()));
	}
}
