
#include <string>
#include <stdexcept>

using std::string;

// thrown if the reader chokes and dies
struct ReaderException : public std::runtime_error {
	string reason;
	public:
		ReaderException(string _reason) : std::runtime_error(_reason) {
			reason = _reason;
		}
		const char* what() const throw() {
			return reason.c_str();
		}
		~ReaderException() throw() {
		}
};

