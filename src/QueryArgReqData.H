// Copyright 2015 CBS Interactive Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// CBS Interactive accepts contributions to software products and free
// and open-source projects owned, licensed, managed, or maintained by
// CBS Interactive submitted under the terms of the CBS Interactive
// Contribution License Agreement (the "Contribution Agreement"); you may
// not submit software to CBS Interactive for inclusion in a CBS
// Interactive product or project unless you agree to the terms of the
// CBS Interactive Contribution License Agreement or have executed a
// separate agreement with CBS Interactive governing the use of such
// submission. A copy of the Contribution Agreement should have been
// included with the software. You may also obtain a copy of the
// Contribution Agreement at
// http://www.cbsinteractive.com/cbs-interactive-software-grant-and-contribution-license-agreement/.



#ifndef RUM_QUERYARGREQDATA_H
#define RUM_QUERYARGREQDATA_H


#include "ReqData.H"
#include "StrPtrMap.H"
#include "StrVec.H"
#include "StrBuffer.H"



namespace rum
{

    ///
    /// QueryArgReqData stores a request's query arguments
    /// and their values.  Because the same query argument can appear
    /// multiple times with different values, a hash map is used where
    /// the key is the query argument name, and the value is a vector
    /// of strings.
    ///
    class QueryArgReqData : public ReqData
    {
    public:
        QueryArgReqData(apr_pool_t *p, ReqCtx *reqCtx__)
            : ReqData(p, reqCtx__),
              queryArgValVecMap_(pool()),
              queryArgRawValVecMap_(pool()),
              isParsedQueryArgs_(false),
              isParsedRawQueryArgs_(false)
            { }



        ///
        /// Retrieve the map holding the query arg names and their
        /// associated value vectors.  The first time this method is
        /// called, the query string is parsed, and subsequent times
        /// the cached data is returned.
        ///
        const StrPtrMap<StrVec *>& queryArgValVecMap();



        ///
        /// Retrieve the map holding the query arg names and their
        /// associated raw (undecoded) value vectors.  The first time
        /// this method is called, the query string is parsed, and
        /// subsequent times the cached data is returned.
        ///
        const StrPtrMap<StrVec *>& queryArgRawValVecMap();



        ///
        /// Retrieve either the map holding the query arg names and
        /// their associated raw (undecoded) value vectors, or the map
        /// holding the decoded value vectors.  This is used when only
        /// the list of keys are needed.
        ///
        const StrPtrMap<StrVec *>& queryArgAnyValVecMap();



        ///
        /// Retrieve the vector holding the values of the given query
        /// arg name.
        ///
        /// @param qaName the query arg name
        ///
        /// @return vector of query arg values
        ///
        const StrVec *queryArgValVec(const char *qaName);



        ///
        /// Retrieve the vector holding the raw (undecoded) values of
        /// the given query arg name.
        ///
        /// @param qaName the query arg name
        ///
        /// @return vector of raw (undecoded) query arg values
        ///
        const StrVec *queryArgRawValVec(const char *qaName);



        virtual StrBuffer& write(StrBuffer& sb) const;



        ///
        /// Reset the queryArgValVecMap and queryArgRawValVecMap so
        /// that on subsequent access to them the query args will be
        /// re-parsed.
        ///
        virtual void reset();



    private:
        StrPtrMap<StrVec *> queryArgValVecMap_;
        StrPtrMap<StrVec *> queryArgRawValVecMap_;
        bool isParsedQueryArgs_;
        bool isParsedRawQueryArgs_;



        QueryArgReqData(const QueryArgReqData& /*from*/)
            : ReqData(0, 0),
              queryArgValVecMap_(0),
              queryArgRawValVecMap_(0),
              isParsedQueryArgs_(false),
              isParsedRawQueryArgs_(false)
            { }



        QueryArgReqData&
        operator=(const QueryArgReqData& that)
            {
                PoolAllocated::operator=(that);
                return *this;
            }



        void parse(bool raw);

    };

}


#endif // RUM_QUERYARGREQDATA_H
