/**
 * @file
 * @brief Source file for PlayerPrivate class
 * @author Duzy Chan <code@duzy.info>
 *
 * @section LICENSE
 *
 * Copyright (c) 2008-2013 OpenShot Studios, LLC
 * (http://www.openshotstudios.com). This file is part of
 * OpenShot Library (http://www.openshot.org), an open-source project
 * dedicated to delivering high quality video editing and animation solutions
 * to the world.
 *
 * OpenShot Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenShot Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenShot Library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../include/ReaderBase.h"
#include "../include/RendererBase.h"
#include "../include/AudioReaderSource.h"
#include "PlayerPrivate.h"
#include "AudioPlaybackThread.h"
#include "VideoPlaybackThread.h"

namespace openshot
{
    PlayerPrivate::PlayerPrivate(RendererBase *rb)
	: Thread("player"), position(0)
	, audioPlayback(new AudioPlaybackThread())
	, videoPlayback(new VideoPlaybackThread(rb))
    {
    }

    PlayerPrivate::~PlayerPrivate()
    {
	if (isThreadRunning()) stopThread(500);
	if (audioPlayback->isThreadRunning()) audioPlayback->stopThread(500);
	if (videoPlayback->isThreadRunning()) videoPlayback->stopThread(500);
	delete audioPlayback;
	delete videoPlayback;
    }

    void PlayerPrivate::run()
    {
	if (audioPlayback->isThreadRunning()) audioPlayback->stopThread(-1);
	if (videoPlayback->isThreadRunning()) videoPlayback->stopThread(-1);

	audioPlayback->setReader(reader);
	audioPlayback->startThread(1);
	videoPlayback->startThread(2);

	//tr1::shared_ptr<Frame> frame = getFrame();
	tr1::shared_ptr<Frame> frame;
	while (!threadShouldExit()) {
	    const Time t1 = Time::getCurrentTime();

	    frame = audioPlayback->getFrame();
	    if (!frame) {
		sleep(1); continue;
		//break;
	    }

	    videoPlayback->frame = frame;
	    videoPlayback->render.signal();
	    //frame = getFrame();
	    videoPlayback->rendered.wait();

	    const Time t2 = Time::getCurrentTime();
	    double ft = (1000.0 / reader->info.fps.ToDouble());
	    int64 d = t2.toMilliseconds() - t1.toMilliseconds();
	    int st = int(ft - d + 0.5);
	    if (0 < ft - d) sleep(st);
	    std::cout << "frametime: " << ft << " - " << d << " = " << st << std::endl;
	}
	
	if (audioPlayback->isThreadRunning()) audioPlayback->stopThread(-1);
	if (videoPlayback->isThreadRunning()) videoPlayback->stopThread(-1);
    }

    tr1::shared_ptr<Frame> PlayerPrivate::getFrame()
    {
	try {
	    return reader->GetFrame(position++);
	} catch (const ReaderClosed & e) {
	    // ...
	} catch (const TooManySeeks & e) {
	    // ...
	} catch (const OutOfBoundsFrame & e) {
	    // ...
	}
	return tr1::shared_ptr<Frame>();
    }

    bool PlayerPrivate::startPlayback()
    {
	if (position < 0) return false;
	stopPlayback(-1);
	startThread(1);
	return true;
    }

    void PlayerPrivate::stopPlayback(int timeOutMilliseconds)
    {
	if (isThreadRunning()) stopThread(timeOutMilliseconds);
    }

}