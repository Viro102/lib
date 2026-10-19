#include <qdir.h>
#include <qregularexpression.h>

#include <fcntl.h>
#include <unistd.h>

#include "caelestia/config/paths.hpp"
#include "caelestia/theme/theme.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

namespace {

QString ansi(m3colours::Argb colour, const QString& code) {
    return u"\x1b]%1;rgb:%2/%3/%4\x1b\\"_s.arg(
        code, hex(colour).sliced(0, 2), hex(colour).sliced(2, 2), hex(colour).sliced(4, 2));
}

QString ansi(m3colours::Argb colour, int code) {
    return ansi(colour, QString::number(code));
}

QString ansi(m3colours::Argb colour, int code, int index) {
    return ansi(colour, u"%1;%2"_s.arg(code).arg(index));
}

void writeToPty(const QString& path, const QByteArray& data) {
    const int fd = ::open(path.toLocal8Bit().constData(), O_WRONLY | O_NONBLOCK | O_NOCTTY);
    if (fd < 0)
        return;
    [[maybe_unused]] const auto written = ::write(fd, data.constData(), static_cast<size_t>(data.size()));
    ::close(fd);
}

} // namespace

QString Theme::sequences() const {
    const auto& c = m_scheme.colours;
    return ansi(c.onSurface, 10) + ansi(c.surface, 11) + ansi(c.secondary, 12) + ansi(c.secondary, 17) +
           ansi(c.term0, 4, 0) + ansi(c.term1, 4, 1) + ansi(c.term2, 4, 2) + ansi(c.term3, 4, 3) + ansi(c.term4, 4, 4) +
           ansi(c.term5, 4, 5) + ansi(c.term6, 4, 6) + ansi(c.term7, 4, 7) + ansi(c.term8, 4, 8) + ansi(c.term9, 4, 9) +
           ansi(c.term10, 4, 10) + ansi(c.term11, 4, 11) + ansi(c.term12, 4, 12) + ansi(c.term13, 4, 13) +
           ansi(c.term14, 4, 14) + ansi(c.term15, 4, 15) + ansi(c.primary, 4, 16) + ansi(c.secondary, 4, 17) +
           ansi(c.tertiary, 4, 18);
}

void Theme::applyTerminal() const {
    const auto seqs = sequences();
    writeFile(config::ConfigPaths::stateDir() + u"/sequences.txt"_s, seqs);

    const auto data = seqs.toUtf8();
    static const QRegularExpression k_digits(u"^\\d+$"_s);
    const QDir pts(u"/dev/pts"_s);
    for (const auto& name : pts.entryList(QDir::System | QDir::Files | QDir::NoDotAndDotDot)) {
        if (k_digits.match(name).hasMatch())
            writeToPty(pts.filePath(name), data);
    }
}

} // namespace caelestia::theme
