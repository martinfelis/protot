#pragma once

#include<unordered_map>
#include<vector>

struct AFileModificationListener {
	virtual bool OnFileChanged(const std::string& filename) = 0;
};

struct FileModificationObserver {
	struct FileObserverInfo {
		ino_t mFileId = 0;
		int mFileMTime = 0;
		int mFileMTimeNSec = 0;
		off_t mFileSize = 0;
		std::vector<AFileModificationListener*> mListeners;
	};

	typedef std::unordered_map<std::string, FileObserverInfo > FilenameListenerMap;
	FilenameListenerMap mObserverInfos;

	void AddListener(const std::string &filename, AFileModificationListener* listener) {
		FilenameListenerMap::iterator iter;
		iter = mObserverInfos.find(filename);

		if (iter == mObserverInfos.end()) {
			FileObserverInfo observer_info;
			observer_info.mListeners.push_back(listener);
			UpdateFileInfo(filename, observer_info);
			mObserverInfos[filename] = observer_info;
		} else {
			iter->second.mListeners.push_back(listener);
		}
	}

	void Trigger(const std::string &filename) {
		FilenameListenerMap::iterator iter;
		iter = mObserverInfos.find(filename);

		assert (iter != mObserverInfos.end());

		std::vector<AFileModificationListener*>::iterator listener_iter = iter->second.mListeners.begin();
		while (listener_iter != iter->second.mListeners.end()) {
			if (!(*listener_iter)->OnFileChanged(filename))
				break;

			listener_iter++;
		}
	}

	bool UpdateFileInfo(const std::string& filename, FileObserverInfo& observer_info) {
		struct stat attr;
		bool stat_result = stat(filename.c_str(), &attr);

		if (stat_result != 0) {
			gLog ("Error: could not stat watched file %s", filename.c_str());
			abort();
		}

		if (   observer_info.mFileId != attr.st_ino 
				|| observer_info.mFileMTime != attr.st_mtime
				|| observer_info.mFileMTimeNSec != attr.st_mtim.tv_nsec
				|| observer_info.mFileSize != attr.st_size
				|| observer_info.mFileSize == 0 
			 ) {
			observer_info.mFileId = attr.st_ino;
			observer_info.mFileMTime = attr.st_mtime;
			observer_info.mFileMTimeNSec = attr.st_mtim.tv_nsec;
			observer_info.mFileSize = attr.st_size;

			return true;
		}
		return false;
	}

	void CheckFileModification(const std::string& filename, FileObserverInfo& observer_info) {
		if (UpdateFileInfo(filename, observer_info)) {
			gLog ("Detected file change of %s: new size %d",
					filename.c_str(), observer_info.mFileSize);
			Trigger(filename);
		}
	}

	void Update() {
		FilenameListenerMap::iterator iter = mObserverInfos.begin();

		while (iter != mObserverInfos.end()) {
			CheckFileModification(iter->first, iter->second);
			iter++;
		}
	}
};
