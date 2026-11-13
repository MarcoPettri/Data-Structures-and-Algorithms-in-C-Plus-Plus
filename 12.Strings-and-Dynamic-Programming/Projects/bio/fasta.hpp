/**
 * @file fasta.hpp
 * @brief Minimal, fast FASTA reader suited to GenBank / NCBI nucleotide downloads.
 *
 * Sequences can be downloaded from NCBI GenBank in FASTA format, e.g.
 * @code
 * https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?db=nuccore&id=NC_012920.1&rettype=fasta&retmode=text
 * @endcode
 * or via the "Send to -> Complete Record -> File -> FASTA" option on the
 * nuccore web page.
 */

#ifndef BIO_FASTA_HPP
#define BIO_FASTA_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace bio {

/**
 * @brief One FASTA entry.
 */
struct FastaRecord {
    std::string id;           ///< Text after '>' up to the first whitespace (e.g. "NC_012920.1").
    std::string description;  ///< Remainder of the header line.
    std::string sequence;     ///< Upper-case residues with whitespace removed.
};

/**
 * @brief Parses all records from a stream.
 *
 * Handling rules:
 *  - CR/LF and LF line endings are accepted; blank lines and lines starting
 *    with ';' are ignored.
 *  - Residues are upper-cased; '-' and '*' are dropped.
 *  - Any other non-alphabetic character is an error.
 *
 * @param in  Input stream.
 * @return All records in file order.
 *
 * @throws std::runtime_error on sequence data before the first header or on an
 *         invalid character (the message includes the line number).
 */
[[nodiscard]] std::vector<FastaRecord> readFasta(std::istream& in);

/**
 * @brief Parses all records from a file.
 *
 * @param path  File system path.
 * @return All records in file order.
 *
 * @throws std::runtime_error if the file cannot be opened or is malformed.
 */
[[nodiscard]] std::vector<FastaRecord> readFastaFile(const std::string& path);

}  // namespace bio

#endif  // BIO_FASTA_HPP