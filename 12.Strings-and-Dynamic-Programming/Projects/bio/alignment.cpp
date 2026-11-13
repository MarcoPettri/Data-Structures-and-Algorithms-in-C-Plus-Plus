/**
 * @file alignment.cpp
 * @brief Implementation of the LCS-based alignment builder and printer.
 */

#include "alignment.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace bio {

double Alignment::identity() const noexcept
{
    return top.empty() ? 0.0 : static_cast<double>(matches) / static_cast<double>(top.size());
}

Alignment buildAlignment(std::string_view a, std::string_view b, const std::vector<Match>& lcsPairs)
{
    Alignment aln;
    const std::size_t capacity = a.size() + b.size();
    aln.top.reserve(capacity);
    aln.bottom.reserve(capacity);
    aln.midline.reserve(capacity);

    std::size_t ia = 0;
    std::size_t ib = 0;

    // Emits the unmatched stretch a[ia, endA) / b[ib, endB): mismatches first, then gaps.
    const auto emitGapRegion = [&](std::size_t endA, std::size_t endB) {
        const std::size_t lenA = endA - ia;
        const std::size_t lenB = endB - ib;
        const std::size_t common = std::min(lenA, lenB);

        for (std::size_t k = 0; k < common; ++k) {
            aln.top.push_back(a[ia + k]);
            aln.bottom.push_back(b[ib + k]);
            aln.midline.push_back(' ');
        }
        aln.mismatches += common;

        for (std::size_t k = common; k < lenA; ++k) {
            aln.top.push_back(a[ia + k]);
            aln.bottom.push_back('-');
            aln.midline.push_back(' ');
        }
        for (std::size_t k = common; k < lenB; ++k) {
            aln.top.push_back('-');
            aln.bottom.push_back(b[ib + k]);
            aln.midline.push_back(' ');
        }
        aln.gaps += (lenA - common) + (lenB - common);

        ia = endA;
        ib = endB;
    };

    for (const auto& [ma, mb] : lcsPairs) {
        if (ma < ia || mb < ib || ma >= a.size() || mb >= b.size() || a[ma] != b[mb]) {
            throw std::invalid_argument("buildAlignment: invalid LCS pair list");
        }
        emitGapRegion(ma, mb);
        aln.top.push_back(a[ma]);
        aln.bottom.push_back(b[mb]);
        aln.midline.push_back('|');
        ++aln.matches;
        ++ia;
        ++ib;
    }
    emitGapRegion(a.size(), b.size());
    return aln;
}

Alignment alignByLcs(std::string_view a, std::string_view b)
{
    return buildAlignment(a, b, lcsMatches(a, b));
}

std::string lcsString(std::string_view a, const std::vector<Match>& lcsPairs)
{
    std::string out;
    out.reserve(lcsPairs.size());
    for (const auto& pair : lcsPairs) {
        out.push_back(a.at(pair.first));
    }
    return out;
}

void writeAlignment(std::ostream& os, const Alignment& alignment, std::size_t lineWidth)
{
    lineWidth = std::max<std::size_t>(lineWidth, 1);
    std::size_t posTop = 1;     // One-based position of the next residue in each sequence.
    std::size_t posBottom = 1;

    for (std::size_t start = 0; start < alignment.length(); start += lineWidth) {
        const std::size_t count = std::min(lineWidth, alignment.length() - start);
        const auto top = std::string_view(alignment.top).substr(start, count);
        const auto mid = std::string_view(alignment.midline).substr(start, count);
        const auto bottom = std::string_view(alignment.bottom).substr(start, count);

        os << "A " << std::setw(10) << posTop << "  " << top << '\n'
           << "  " << std::setw(10) << ' ' << "  " << mid << '\n'
           << "B " << std::setw(10) << posBottom << "  " << bottom << "\n\n";

        posTop += static_cast<std::size_t>(std::count_if(top.begin(), top.end(),
                                                         [](char c) { return c != '-'; }));
        posBottom += static_cast<std::size_t>(std::count_if(bottom.begin(), bottom.end(),
                                                            [](char c) { return c != '-'; }));
    }
}

}  // namespace bio