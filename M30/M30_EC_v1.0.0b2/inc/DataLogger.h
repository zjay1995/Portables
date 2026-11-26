/*
 *  ____ ___ ____       _                _
 * |  _ \_ _|  _ \     / \   _ __   __ _| |_   _ _______ _ __
 * | |_) | || | | |   / _ \ | '_ \ / _` | | | | |_  / _ \ '__|
 * |  __/| || |_| |  / ___ \| | | | (_| | | |_| |/ /  __/ |
 * |_|  |___|____/  /_/   \_\_| |_|\__,_|_|\__, /___\___|_|
 *                                         |___/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/DataLogger.h
 * Description : Data logger interface.
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
#pragma once

#include <SPIFFS.h>

#include <time.h>
#include <sys/time.h>

class GasManager;
class DataSource;

class DataLogger
{
	int m_singleFileLimit = 10;
	int m_maxSize;
	File m_file;

	bool m_sessionRunning = false;

	int m_currentValueCount = 0;

	int m_secondsBetweenDataPoints = 1;

	unsigned long m_lastTickMillis = 0;

	DataSource* m_dataSource;

	String m_currentSessionTimestamp;
	int m_fileIndex = 0;
	String m_currentFileName;

	GasManager* m_gasManager;

	String createFileName(String ts, String gas, int secondsBetweenDataPoints, int fileIndex);

	String getCurrentDateTimeStr();

	void checkIfGracefulShutdown();

	bool shouldExecTick();

public:

	DataLogger()=default;
	~DataLogger()=default;


	void init(DataSource* dataSource, GasManager* gasManager);

	void handleTick();

	bool startFlashStoreSession();
	bool stopFlashStoreSession();

	bool isFlashStoreSessionRunning() const;

public:

	static portMUX_TYPE s_mtxLogger;

};