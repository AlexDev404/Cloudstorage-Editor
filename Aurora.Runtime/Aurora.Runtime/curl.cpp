#include "curl.h"
#include "util.h"
#include "build.h"
#include <windows.h>
#include <string>
#include <regex>

namespace aurora {
	const std::regex rEpicGames("https:\\/\\/(.*)\\.ol\\.epicgames.com");
	const std::regex CloudstorageSystem("/fortnite/api/cloudstorage/system");
	const std::regex DefaultGame("/fortnite/api/cloudstorage/system/3460cbe1c57d4a838ace32951a4d7171");
	const std::regex CustomName("/account/api/public/account/(.*)");

	INT (*CurlSetopt)(LPVOID, INT, va_list) = NULL;
	INT CurlSetoptVa(LPVOID lpContext, INT iOption, ...) {
		va_list list{};
		va_start(list, iOption);

		INT iResult = CurlSetopt(lpContext, iOption, list);

		va_end(list);

		return iResult;
	}

	INT CurlEasySetopt(LPVOID lpContext, INT iTag, ...) {
		if (!lpContext) {
			return 43; // CURLE_BAD_FUNCTION_ARGUMENT
		}

		va_list list{}, copy{}; // Copy only exists for our tag overrides.
		va_start(list, iTag);
		va_copy(copy, list);

		INT iResult = 0;
		switch (iTag) {
		case 64: // CURLOPT_SSL_VERIFYPEER
			iResult = CurlSetoptVa(lpContext, iTag, FALSE); // Disables VerifyPeer.
			break;

		// Fuck you C++, I hate you.
		case 10002: // CURLOPT_URL
		{
			std::string sUrl(va_arg(copy, PCHAR));

			if (std::regex_search(sUrl, CloudstorageSystem)) {
				sUrl = std::regex_replace(sUrl, rEpicGames, HOST_URL);
			}
			else if (std::regex_search(sUrl, DefaultGame)) {
				sUrl = std::regex_replace(sUrl, rEpicGames, HOST_URL);
			}
			else if (std::regex_search(sUrl, CustomName))
			{
				sUrl = std::regex_replace(sUrl, rEpicGames, HOST_URL);
			}

			iResult = CurlSetoptVa(lpContext, iTag, sUrl.c_str());
			break;
		}

#ifdef _PROD
		case 10004: // CURLOPT_PROXY
			iResult = CurlSetoptVa(lpContext, iTag, ""); // Disables Proxy.
			break;
#endif

		default: // Everything else.
			iResult = CurlSetopt(lpContext, iTag, list);
			break;
		}

		va_end(copy);
		va_end(list);

		return iResult;
	}

	Curl::Curl() {
		auto pCurlSetoptAddress = Util::FindPattern
		(
			"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x30\x33\xED\x49\x8B\xF0\x48\x8B\xD9",
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		);
		if (!pCurlSetoptAddress) {
			printf("Finding pattern for CurlSetopt has failed, exiting immediately!\n");
			exit(EXIT_FAILURE);
		}

		CurlSetopt = reinterpret_cast<decltype(CurlSetopt)>(pCurlSetoptAddress);

		auto pCurlEasySetoptAddress = Util::FindPattern
		(
			"\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x48\x83\xEC\x28\x48\x85\xC9\x75\x08\x8D\x41\x2B\x48\x83\xC4\x28\xC3\x4C",
			"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
		);
		if (!pCurlEasySetoptAddress) {
			printf("Finding pattern for CurlEasySetopt has failed, exiting immediately!\n");
			exit(EXIT_FAILURE);
		}

		m_pCurlEasySetoptHook = new Hook(reinterpret_cast<uintptr_t>(pCurlEasySetoptAddress), reinterpret_cast<uintptr_t>(CurlEasySetopt));
		if (!m_pCurlEasySetoptHook->bSuccess) {
			printf("Initializing hook for CurlEasySetopt has failed, exiting immediately!\n");
			exit(EXIT_FAILURE);
		}
	}
	Curl::~Curl() {
		if (m_pCurlEasySetoptHook) {
			delete m_pCurlEasySetoptHook;
		}
	}
}
