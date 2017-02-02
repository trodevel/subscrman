/*

Subscription manager.

Copyright (C) 2017 Sergey Kolevatov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

*/


// $Revision: 5643 $ $Date:: 2017-02-01 #$ $Author: serge $

#ifndef SUBSCR_MAN_T_H
#define SUBSCR_MAN_T_H

#include <map>                          // std::map
#include <string>                       // std::string

#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT

namespace subscrman
{

template <class SUBS, typename SUBS_ID, typename REQ_ID>
class SubscrManT
{
protected:

    bool init( const std::string & name );

    bool add( const SUBS_ID subs_id, SUBS subs );
    bool remove( const SUBS_ID subs_id );

    template <class OBJ>
    void forward_request_to_subs( const OBJ * obj, const SUBS_ID subs_id, const REQ_ID req_id );

    template <class OBJ>
    void forward_response_to_subs( const OBJ * obj, const REQ_ID req_id );

    template <class OBJ>
    void forward_event_to_subs( const OBJ * obj, const SUBS_ID subs_id );

private:

    void remove_if_closed( SUBS subs, const SUBS_ID subs_id );

protected:

    typedef std::map<SUBS_ID, SUBS>     MapSubsIdToSubs;
    typedef std::map<REQ_ID, SUBS_ID>   MapReqIdToSubsId;

protected:

    MapSubsIdToSubs             map_subs_id_to_subs_;
    MapReqIdToSubsId            map_req_id_to_subs_id_;

private:
    std::string                 name_;
};

template <class SUBS, typename SUBS_ID, typename REQ_ID>
template <class OBJ>
void SubscrManT<SUBS,SUBS_ID,REQ_ID>::forward_request_to_subs( const OBJ * obj, const SUBS_ID subs_id, const REQ_ID req_id )
{
    auto it = map_subs_id_to_subs_.find( subs_id );

    if( it == map_subs_id_to_subs_.end() )
    {
        dummy_log_info( name_, "cannot forward to subs, subs id %u not found", subs_id );
        return;
    }

    auto subs = it->second;

    auto _b = map_req_id_to_subs_id_.insert( MapReqIdToSubsId::value_type( req_id, subs_id ) ).second;

    ASSERT( _b );

    subs->handle( obj );

    remove_if_closed( subs, subs_id );
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
bool SubscrManT<SUBS,SUBS_ID,REQ_ID>::init( const std::string & name )
{
    name_   = name;
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
bool SubscrManT<SUBS,SUBS_ID,REQ_ID>::add( const SUBS_ID subs_id, SUBS subs )
{
    auto b = map_subs_id_to_subs_.insert( MapSubsIdToSubs::value_type( subs_id, subs ) ).second;

    if( b == false )
    {
        dummy_log_info( name_, "cannot add subs, subs id %u already exists", subs_id );
        return false;
    }

    dummy_log_debug( name_, "added subs id %u", subs_id );

    return true;
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
bool SubscrManT<SUBS,SUBS_ID,REQ_ID>::remove( const SUBS_ID subs_id )
{
    auto it = map_subs_id_to_subs_.find( subs_id );

    if( it == map_subs_id_to_subs_.end() )
    {
        dummy_log_info( name_, "cannot remove subs, subs id %u not found", subs_id );
        return false;
    }

    map_subs_id_to_subs_.erase( it );

    dummy_log_debug( name_, "removed subs id %u", subs_id );

    return true;
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
template <class OBJ>
void SubscrManT<SUBS,SUBS_ID,REQ_ID>::forward_response_to_subs( const OBJ * obj, const REQ_ID req_id )
{
    auto it = map_req_id_to_subs_id_.find( req_id );

    if( it == map_req_id_to_subs_id_.end() )
    {
        dummy_log_info( name_, "cannot forward to subs, req id %u not found", req_id );
        return;
    }

    auto subs_id = it->second;

    map_req_id_to_subs_id_.erase( it );    // erase request after getting response

    forward_event_to_subs( obj, subs_id );
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
template <class OBJ>
void SubscrManT<SUBS,SUBS_ID,REQ_ID>::forward_event_to_subs( const OBJ * obj, const SUBS_ID subs_id )
{
    auto it = map_subs_id_to_subs_.find( subs_id );

    if( it == map_subs_id_to_subs_.end() )
    {
        dummy_log_info( name_, "cannot forward to subs, subs id %u not found", subs_id );
        return;
    }

    auto subs = it->second;

    subs->handle( obj );

    remove_if_closed( subs, subs_id, 0 );
}

template <class SUBS, typename SUBS_ID, typename REQ_ID>
void SubscrManT<SUBS,SUBS_ID,REQ_ID>::remove_if_closed( SUBS subs, const SUBS_ID subs_id )
{
    if( subs->is_closed() )
    {
        remove( subs_id );
    }
}

} // namespace subscrman

#endif  // SUBSCR_MAN_T_H
