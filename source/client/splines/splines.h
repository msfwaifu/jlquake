//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef __SPLINES_H
#define __SPLINES_H

#include "../../common/strings.h"
#include "../../common/mathlib.h"
#include "../../common/math/Vec3.h"

class idSplineList {
public:
	idSplineList() {
		clear();
	}

	idSplineList( const char* p ) {
		clear();
		name = p;
	};

	~idSplineList() {
		clear();
	};

	void clearControl() {
		for ( int i = 0; i < controlPoints.Num(); i++ ) {
			delete controlPoints[ i ];
		}
		controlPoints.Clear();
	}

	void clearSpline() {
		for ( int i = 0; i < splinePoints.Num(); i++ ) {
			delete splinePoints[ i ];
		}
		splinePoints.Clear();
	}

	void parse( const char** text );

	void clear() {
		clearControl();
		clearSpline();
		splineTime.Clear();
		dirty = true;
		activeSegment = 0;
		granularity = 0.025;
	}

	void initPosition( long startTime, long totalTime );
	const idVec3* getPosition( long time );


	void addPoint( float x, float y, float z ) {
		controlPoints.Append( new idVec3( x, y, z ) );
		dirty = true;
	}

	void buildSpline();

	void setGranularity( float f ) {
		granularity = f;
	}

	float getGranularity() {
		return granularity;
	}

	idVec3* getSegmentPoint( int index ) {
		assert( index >= 0 && index < splinePoints.Num() );
		return splinePoints[ index ];
	}


	void setSegmentTime( int index, int time ) {
		assert( index >= 0 && index < splinePoints.Num() );
		splineTime[ index ] = time;
	}

	int getSegmentTime( int index ) {
		assert( index >= 0 && index < splinePoints.Num() );
		return ( int )splineTime[ index ];
	}
	void addSegmentTime( int index, int time ) {
		assert( index >= 0 && index < splinePoints.Num() );
		splineTime[ index ] += time;
	}

	float totalDistance();

	static idVec3 zero;

	int getActiveSegment() {
		return activeSegment;
	}

	void setActiveSegment( int i ) {
		//assert(i >= 0 && (splinePoints.Num() > 0 && i < splinePoints.Num()));
		activeSegment = i;
	}

	int numSegments() {
		return splinePoints.Num();
	}

	const char* getName() {
		return name.CStr();
	}

	void setName( const char* p ) {
		name = p;
	}

	bool validTime() {
		if ( dirty ) {
			buildSpline();
		}
		// gcc doesn't allow static casting away from bools
		// why?  I've no idea...
		return ( bool )( splineTime.Num() > 0 && splineTime.Num() == splinePoints.Num() );
	}

	void setTime( long t ) {
		time = t;
	}

	void setBaseTime( long t ) {
		baseTime = t;
	}

protected:
	idStr name;
	float calcSpline( int step, float tension );
	idList<idVec3*> controlPoints;
	idList<idVec3*> splinePoints;
	idList<double> splineTime;
	float granularity;
	bool dirty;
	int activeSegment;
	long baseTime;
	long time;
	friend class idCamera;
};

// time in milliseconds
// velocity where 1.0 equal rough walking speed
struct idVelocity {
	idVelocity( long start, long duration, float s ) {
		startTime = start;
		time = duration;
		speed = s;
	}
	long startTime;
	long time;
	float speed;
};

// can either be a look at or origin position for a camera
//
class idCameraPosition {
public:

	virtual void clearVelocities() {
		for ( int i = 0; i < velocities.Num(); i++ ) {
			delete velocities[ i ];
			velocities[ i ] = NULL;
		}
		velocities.Clear();
	}

	virtual void clear() {
		clearVelocities();
	}

	idCameraPosition( const char* p ) {
		name = p;
	}

	idCameraPosition() {
		time = 0;
		name = "position";
	}

	idCameraPosition( long t ) {
		time = t;
	}

	virtual ~idCameraPosition() {
		clear();
	}


// this can be done with RTTI syntax but i like the derived classes setting a type
// makes serialization a bit easier to see
//
	enum positionType {
		FIXED = 0x00,
		INTERPOLATED,
		SPLINE,
		POSITION_COUNT
	};


	virtual void start( long t ) {
		startTime = t;
	}

	long getTime() {
		return time;
	}

	virtual void setTime( long t ) {
		time = t;
	}

	float getBaseVelocity() {
		return baseVelocity;
	}

	float getVelocity( long t ) {
		long check = t - startTime;
		for ( int i = 0; i < velocities.Num(); i++ ) {
			if ( check >= velocities[ i ]->startTime && check <= velocities[ i ]->startTime + velocities[ i ]->time ) {
				return velocities[ i ]->speed;
			}
		}
		return baseVelocity;
	}

	void addVelocity( long start, long duration, float speed ) {
		velocities.Append( new idVelocity( start, duration, speed ) );
	}

	virtual const idVec3* getPosition( long t ) {
		assert( true );
		return NULL;
	}

	virtual void parse( const char** text ) {};
	virtual bool parseToken( const char* key, const char** text );

	const char* getName() {
		return name.CStr();
	}

	void setName( const char* p ) {
		name = p;
	}

	void calcVelocity( float distance ) {
		if ( time ) {	//DAJ BUGFIX
			float secs = ( float )time / 1000;
			baseVelocity = distance / secs;
		}
	}

protected:
	long startTime;
	long time;
	idCameraPosition::positionType type;
	idStr name;
	idList<idVelocity*> velocities;
	float baseVelocity;
};

class idFixedPosition : public idCameraPosition {
public:
	void init() {
		pos.Zero();
		type = idCameraPosition::FIXED;
	}

	idFixedPosition() : idCameraPosition() {
		init();
	}

	idFixedPosition( idVec3 p ) : idCameraPosition() {
		init();
		pos = p;
	}

	virtual void addPoint( const float x, const float y, const float z ) {
		pos.Set( x, y, z );
	}


	~idFixedPosition() {
	}

	virtual const idVec3* getPosition( long t ) {
		return &pos;
	}

	void parse( const char** text );

protected:
	idVec3 pos;
};

class idInterpolatedPosition : public idCameraPosition {
public:
	void init() {
		type = idCameraPosition::INTERPOLATED;
		first = true;
		startPos.Zero();
		endPos.Zero();
	}

	idInterpolatedPosition() : idCameraPosition() {
		init();
	}

	idInterpolatedPosition( idVec3 start, idVec3 end, long time ) : idCameraPosition( time ) {
		init();
		startPos = start;
		endPos = end;
	}

	~idInterpolatedPosition() {
	}

	virtual const idVec3* getPosition( long t );

	void parse( const char** text );

	virtual void addPoint( const float x, const float y, const float z ) {
		if ( first ) {
			startPos.Set( x, y, z );
			first = false;
		} else {
			endPos.Set( x, y, z );
			first = true;
		}
	}

	virtual void start( long t ) {
		idCameraPosition::start( t );
		lastTime = startTime;
		distSoFar = 0.0;
		idVec3 temp = startPos;
		temp -= endPos;
		calcVelocity( temp.Length() );
	}

protected:
	bool first;
	idVec3 startPos;
	idVec3 endPos;
	long lastTime;
	float distSoFar;
};

class idSplinePosition : public idCameraPosition {
public:
	void init() {
		type = idCameraPosition::SPLINE;
	}

	idSplinePosition() : idCameraPosition() {
		init();
	}

	idSplinePosition( long time ) : idCameraPosition( time ) {
		init();
	}

	~idSplinePosition() {
	}

	virtual void start( long t ) {
		idCameraPosition::start( t );
		target.initPosition( t, time );
		lastTime = startTime;
		distSoFar = 0.0;
		calcVelocity( target.totalDistance() );
	}

	virtual const idVec3* getPosition( long t );

	void parse( const char** text );

	virtual void addPoint( const float x, const float y, const float z ) {
		target.addPoint( x, y, z );
	}

protected:
	idSplineList target;
	long lastTime;
	float distSoFar;
};

class idCameraFOV {
public:
	idCameraFOV() {
		time = 0;
		length = 0;
		fov = 90;
	}

	idCameraFOV( int v ) {
		time = 0;
		length = 0;
		fov = v;
	}

	idCameraFOV( int s, int e, long t ) {
		startFOV = s;
		endFOV = e;
		length = t;
	}


	~idCameraFOV() {
	}

	void setFOV( float f ) {
		fov = f;
	}

	float getFOV( long t ) {
		if ( length ) {
			float percent = ( t - startTime ) / length;
			if ( percent < 0.0 ) {
				percent = 0.0;
			} else if ( percent > 1.0 ) {
				percent = 1.0;
			}
			float temp = endFOV - startFOV;
			temp *= percent;
			fov = startFOV + temp;

			if ( percent == 1.0 ) {
				length = 0.0;
			}
		}
		return fov;
	}

	void start( long t ) {
		startTime = t;
	}

	void reset( float startfov, float endfov, int start, float len ) {
		startFOV = startfov;
		endFOV = endfov;
		startTime = start;
		length = len * 1000;
	}

	void parse( const char** text );

protected:
	float fov;
	float startFOV;
	float endFOV;
	int startTime;
	int time;
	float length;
};

class idCameraEvent {
public:						// parameters
	enum eventType
	{
		EVENT_NA = 0x00,
		EVENT_WAIT,			//
		EVENT_TARGETWAIT,	//
		EVENT_SPEED,		//
		EVENT_TARGET,		// char(name)
		EVENT_SNAPTARGET,	//
		EVENT_FOV,			// int(time), int(targetfov)
		EVENT_CMD,			//
		EVENT_TRIGGER,		//
		EVENT_STOP,			//
		EVENT_CAMERA,		//
		EVENT_FADEOUT,		// int(time)
		EVENT_FADEIN,		// int(time)
		EVENT_FEATHER,		//
		EVENT_COUNT
	};

	idCameraEvent() {
		paramStr = "";
		type = EVENT_NA;
		time = 0;
	}

	idCameraEvent( eventType t, const char* param, long n ) {
		type = t;
		paramStr = param;
		time = n;
	}

	~idCameraEvent() {
	};

	eventType getType() {
		return type;
	}

	const char* getParam() {
		return paramStr.CStr();
	}

	long getTime() {
		return time;
	}

	void setTime( long n ) {
		time = n;
	}

	void parse( const char** text );

	void setTriggered( bool b ) {
		triggered = b;
	}

	bool getTriggered() {
		return triggered;
	}

protected:
	eventType type;
	idStr paramStr;
	long time;
	bool triggered;
};

class idCameraDef {
public:
	void clear() {
		currentCameraPosition = 0;
		cameraRunning = false;
		lastDirection.Zero();
		baseTime = 30;
		activeTarget = 0;
		name = "camera01";
		fov.setFOV( 90 );
		int i;
		for ( i = 0; i < targetPositions.Num(); i++ ) {
			delete targetPositions[ i ];
		}
		for ( i = 0; i < events.Num(); i++ ) {
			delete events[ i ];
		}
		delete cameraPosition;
		cameraPosition = NULL;
		events.Clear();
		targetPositions.Clear();
	}

	idCameraDef() {
		cameraPosition = NULL;
		clear();
	}

	~idCameraDef() {
		clear();
	}

	void addEvent( idCameraEvent::eventType t, const char* param, long time );

	void addEvent( idCameraEvent* event );

	void parse( const char** text );
	bool load( const char* filename );

	void buildCamera();

	static idCameraPosition* newFromType( idCameraPosition::positionType t ) {
		switch ( t ) {
		case idCameraPosition::FIXED: return new idFixedPosition();
		case idCameraPosition::INTERPOLATED: return new idInterpolatedPosition();
		case idCameraPosition::SPLINE: return new idSplinePosition();
		default:
			break;
		}
		;
		return NULL;
	}

	void addTarget( const char* name, idCameraPosition::positionType type );

	idCameraPosition* getActiveTarget() {
		if ( targetPositions.Num() == 0 ) {
			addTarget( NULL, idCameraPosition::FIXED );
		}
		return targetPositions[ activeTarget ];
	}

	idCameraPosition* getActiveTarget( int index ) {
		if ( targetPositions.Num() == 0 ) {
			addTarget( NULL, idCameraPosition::FIXED );
			return targetPositions[ 0 ];
		}
		return targetPositions[ index ];
	}

	int numTargets() {
		return targetPositions.Num();
	}


	void setActiveTargetByName( const char* name ) {
		for ( int i = 0; i < targetPositions.Num(); i++ ) {
			if ( String::ICmp( name, targetPositions[ i ]->getName() ) == 0 ) {
				setActiveTarget( i );
				return;
			}
		}
	}

	void setActiveTarget( int index ) {
		assert( index >= 0 && index < targetPositions.Num() );
		activeTarget = index;
	}

	void startCamera( long t );
	bool getCameraInfo( long time, idVec3& origin, idVec3& direction, float* fv );
	bool getCameraInfo( long time, float* origin, float* direction, float* fv ) {
		idVec3 org, dir;
		org[ 0 ] = origin[ 0 ];
		org[ 1 ] = origin[ 1 ];
		org[ 2 ] = origin[ 2 ];
		dir[ 0 ] = direction[ 0 ];
		dir[ 1 ] = direction[ 1 ];
		dir[ 2 ] = direction[ 2 ];
		bool b = getCameraInfo( time, org, dir, fv );
		origin[ 0 ] = org[ 0 ];
		origin[ 1 ] = org[ 1 ];
		origin[ 2 ] = org[ 2 ];
		direction[ 0 ] = dir[ 0 ];
		direction[ 1 ] = dir[ 1 ];
		direction[ 2 ] = dir[ 2 ];
		return b;
	}

protected:
	idStr name;
	int currentCameraPosition;
	idVec3 lastDirection;
	bool cameraRunning;
	idCameraPosition* cameraPosition;
	idList<idCameraPosition*> targetPositions;
	idList<idCameraEvent*> events;
	idCameraFOV fov;
	int activeTarget;
	float totalTime;
	float baseTime;
	long startTime;
};

#endif
