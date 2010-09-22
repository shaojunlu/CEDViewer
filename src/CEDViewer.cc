/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#ifdef USE_CLHEP  // only if CLHEP is available !
#include "CEDViewer.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/Cluster.h>
#include <EVENT/Track.h>
#include <EVENT/MCParticle.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/SimTrackerHit.h>

#include <EVENT/ReconstructedParticle.h>


#include <UTIL/LCTypedVector.h>

#include "MarlinCED.h"
#include "CLHEP/Vector/ThreeVector.h"

// fix for transition from CLHEP 1.8 to 1.9
namespace CLHEP{}
using namespace CLHEP ;


#include <math.h>
#include <cmath>
#include <cstdlib>

#include <marlin/Global.h>
#include <gear/GEAR.h>
#include <gear/BField.h>
#include <gear/TPCParameters.h>
#include <gear/PadRowLayout2D.h>
#include <gear/CalorimeterParameters.h>

using namespace lcio ;
using namespace marlin ;

#define MCPARTICLE_LAYER 7
#define SIMCALORIMETERHIT_LAYER 3
#define SIMTRACKERHIT_LAYER 2
#define CALORIMETERHIT_LAYER 5
#define TRACKERHIT_LAYER 4
#define CLUSTER_LAYER 7
#define TRACK_LAYER 8
#define TRACKHELIX_LAYER 9
#define RECOPARTICLE_LAYER 7


CEDViewer aCEDViewer ;


/**Helper struct for drawing collections*/
struct DrawParameters{
  DrawParameters(const std::string& colName, int size, int marker, int layer ) :
    ColName( colName ),
    Size( size ),
    Marker( marker ),
    Layer( layer ) {
  } 
  std::string ColName ;
  int Size ;
  int Marker ;
  int Layer ;
};



CEDViewer::CEDViewer() : Processor("CEDViewer") {
  
  // modify processor description
  _description = "CEDViewer: event display of LCIO objects  - based on CED by A.Zhelezov." ;
  
  StringVec cluExample ;
  cluExample.push_back("CalorimeterHits" ) ;
  cluExample.push_back("0" ) ;
  cluExample.push_back("3" ) ;
  
  registerOptionalParameter( "DrawCollection" , 
                             "collection to be displayed ( ColName, marker type[0-2], size)"  ,
                             _drawCollections ,
                             cluExample ,
                             cluExample.size() ) ;

  StringVec layerExample ;
  layerExample.push_back("TPCTracks" ) ;
  layerExample.push_back("0" ) ;
  layerExample.push_back("3" ) ;
  layerExample.push_back("4" ) ;
  
  registerOptionalParameter( "DrawInLayer" , 
                             "collection to be displayed ( ColName, marker type[0-2], size, layer)",
                             _drawCollectionsLayer ,
                             layerExample ,
                             layerExample.size() ) ;
  
  
}

void CEDViewer::init() { 

  // usually a good idea to
  printParameters() ;

  MarlinCED::init(this) ;
  
  _colors.push_back( 0xff00ff );
  _colors.push_back( 0x00ff00 );
  _colors.push_back( 0xffff00 );
  _colors.push_back( 0x0000ff );
  _colors.push_back( 0xff00ff );
  _colors.push_back( 0x00ffff );
  _colors.push_back( 0xffffff );

  _colors.push_back( 0xff88ff );
  _colors.push_back( 0x88ff88 );
  _colors.push_back( 0xffff88 );
  _colors.push_back( 0x8888ff );
  _colors.push_back( 0xff88ff );
  _colors.push_back( 0x88ffff );
  _colors.push_back( 0xffffff );


  _nRun = 0 ;
  _nEvt = 0 ;
  
}

void CEDViewer::processRunHeader( LCRunHeader* run) { 
  _nRun++ ;
} 


void CEDViewer::processEvent( LCEvent * evt ) { 

//-----------------------------------------------------------------------
// Reset drawing buffer and START drawing collection

//hauke hoelbe 08.02.2010
  MarlinCED::newEvent(this) ;
  CEDPickingHandler &pHandler=CEDPickingHandler::getInstance();
  pHandler.update(evt); 

/*
  int i;
  for(i=0;i<25;i++){
    //ced_describe_layer("",i); //delete all old descriptions
    MarlinCED::set_layer_description(std::string(""), i); 

  }
*/

//   ced_new_event();  
//-----------------------------------------------------------------------

  const gear::TPCParameters& gearTPC = Global::GEAR->getTPCParameters() ;
  const gear::PadRowLayout2D& padLayout = gearTPC.getPadLayout() ;

  //add DrawInLayer to
  std::vector< DrawParameters > drawParameters ;
  drawParameters.reserve( _drawCollections.size() + _drawCollectionsLayer.size()  ) ;

  if( parameterSet( "DrawCollection" ) ) {
    
    unsigned index = 0 ;
    while( index < _drawCollections.size() ){

      const std::string & colName = _drawCollections[ index++ ] ;
      int size = std::atoi( _drawCollections[ index++ ].c_str() ) ;
      int marker = std::atoi( _drawCollections[ index++ ].c_str() ) ;
      int layer = -1 ;

      drawParameters.push_back(DrawParameters( colName,size,marker,layer ) ); 
    }
  }
  if( parameterSet( "DrawInLayer" ) ) {
    
    unsigned index = 0 ;
    while( index < _drawCollectionsLayer.size() ){

      const std::string & colName = _drawCollectionsLayer[ index++ ] ;
      int size   = std::atoi( _drawCollectionsLayer[ index++ ].c_str() ) ;
      int marker = std::atoi( _drawCollectionsLayer[ index++ ].c_str() ) ;
      int layer  = std::atoi( _drawCollectionsLayer[ index++ ].c_str() ) ;

      drawParameters.push_back(DrawParameters( colName,size,marker,layer ) ); 
      //std::cout << "layer: " << layer << " description: " << colName << std::endl; //hauke
    }
  }
  
  unsigned nCols = drawParameters.size() ;
  for(unsigned np=0 ; np < nCols ; ++np){
    
    const std::string & colName = drawParameters[np].ColName ;
    int size =   drawParameters[np].Size ;
    int marker = drawParameters[np].Marker ;
    int layer =  drawParameters[np].Layer ;
    
    LCCollection* col = 0 ;
    try{
      
      col = evt->getCollection( colName ) ;
      
    }catch(DataNotAvailableException &e){

      streamlog_out( WARNING ) << " collection " << colName <<  " not found in event:" 
                               << evt->getEventNumber() <<  "  run: " << evt->getRunNumber() << " !" << std::endl ;
      continue ;

    }
    
    if( col->getTypeName() == LCIO::CLUSTER ){
      
      // find Emin and Emax of cluster collection for drawing
      float emin=1.e99, emax=0. ;
      for( int i=0 ; i< col->getNumberOfElements() ; i++ ){
        Cluster* clu = dynamic_cast<Cluster*>( col->getElementAt(i) ) ;
        float e = clu->getEnergy() ;
        if( e > emax) emax = e  ;
        else if( e < emin )  emin = e ;
      }
      streamlog_out( DEBUG )  << " nClu: " << col->getNumberOfElements() 
                              << ", Emin: " << emin << " GeV , Emax: " << emax << " GeV " << std::endl ; 
      
      for( int i=0 ; i< col->getNumberOfElements() ; i++ ){
        
        Cluster* clu = dynamic_cast<Cluster*>( col->getElementAt(i) ) ;
        const CalorimeterHitVec& hits = clu->getCalorimeterHits() ;
        
        layer = ( layer > -1 ? layer : CLUSTER_LAYER ) ;

        //ced_describe_layer( colName.c_str() ,layer);
        MarlinCED::add_layer_description(colName, layer); 



        int ml = marker | ( layer << CED_LAYER_SHIFT ) ;
        int color =  _colors[ i % _colors.size() ] ;
        for( CalorimeterHitVec::const_iterator it = hits.begin();  it != hits.end() ; it++ ) {
          
          //hauke hoelbe: add id for picking!
          ced_hit_ID( (*it)->getPosition()[0],
                      (*it)->getPosition()[1],
                      (*it)->getPosition()[2],
                      ml, size , color, clu->id() ) ;
          
        } // hits
        
        float x = clu->getPosition()[0] ;
        float y = clu->getPosition()[1] ;
        float z = clu->getPosition()[2] ;

        //hauke hoelbe: add id for picking
        ced_hit_ID( x,y,z, ml, size*3 , color, clu->id() ) ;

        Hep3Vector v(x,y,z) ;

        Hep3Vector d(1.,1.,1.) ;


        float length = 100 + 500 * ( clu->getEnergy() - emin )  / ( emax - emin )  ;

        // 	  const FloatVec& pv = clu->getShape() ;
        // 	  if( pv.size() > 7 ){
        // 	      length =  3 * clu->getShape()[3] ;   //FIXME : this is not generic !!!!
        // 	  } else {
        //       std::cout <<   "  ---- no cluster shape[3] :( " << std::endl ;
        //     }

        d.setMag( length  ) ;

        d.setTheta( clu->getITheta() ) ;
        d.setPhi( clu->getIPhi() ) ;
 
        Hep3Vector dp( v + d ) , dm( v - d )   ;

        //hauke hoelbe: need the id for picking mode!
        ced_line_ID( dp.x() , dp.y() , dp.z(),  
                  dm.x() , dm.y() , dm.z(),
                  ml , 1 , color, clu->id() );	 
	  

      } // cluster
	
    } else if( col->getTypeName() == LCIO::TRACK ){

      for( int i=0 ; i< col->getNumberOfElements() ; i++ ){
	  
        Track* trk = dynamic_cast<Track*>( col->getElementAt(i) ) ;
        const TrackerHitVec& hits = trk->getTrackerHits() ;
	  
        layer = ( layer > -1 ? layer : TRACK_LAYER ) ;

        //ced_describe_layer( colName.c_str() ,layer);
        MarlinCED::add_layer_description(colName, layer); 


        int ml = marker | ( layer << CED_LAYER_SHIFT );
	  
        for( TrackerHitVec::const_iterator it = hits.begin();  it != hits.end() ; it++ ) {
	    
          //hauke hoelbe: add id for picking
          ced_hit_ID( (*it)->getPosition()[0],
                      (*it)->getPosition()[1],
                      (*it)->getPosition()[2],
                      ml , size , _colors[ i % _colors.size() ], trk->id() ) ;
          
        } // hits
	  
          // draw the helix:
        double bField = Global::GEAR->getBField().at(  gear::Vector3D(0,0,0)  ).z() ; 
        //double bField = gearTPC.getDoubleVal("tpcBField") ;


        double pt = bField * 3e-4 / std::abs( trk->getOmega() ) ;
        double charge = ( trk->getOmega() > 0. ?  1. : -1. ) ;
	 
        double px = pt * std::cos(  trk->getPhi() ) ;
        double py = pt * std::sin(  trk->getPhi() ) ;
        double pz = pt * trk->getTanLambda() ;


        // double rx = trk->getReferencePoint()[0] ;
        // double ry = trk->getReferencePoint()[1] ;
        // double rz = trk->getReferencePoint()[2] ;

        // start point for drawing ( PCA to reference point )
#ifdef Correct_Track_Params

        double xs = trk->getReferencePoint()[0] -  trk->getD0() * sin( trk->getPhi() ) ;
        double ys = trk->getReferencePoint()[1] +  trk->getD0() * cos( trk->getPhi() ) ;
        double zs = trk->getReferencePoint()[2] +  trk->getZ0() ;

#else  // assume track params have reference point at origin ...

        double xs = 0 -  trk->getD0() * sin( trk->getPhi() ) ;
        double ys = 0 +  trk->getD0() * cos( trk->getPhi() ) ;
        double zs = 0 +  trk->getZ0() ;
#endif        


        layer = ( layer > -1 ? layer : TRACKHELIX_LAYER ) ;

        //ced_describe_layer( colName.c_str() ,layer);
        MarlinCED::add_layer_description(colName, layer); 


        ml = marker | ( layer << CED_LAYER_SHIFT ) ;


        if( pt > 0.01 ) // sanity check
          MarlinCED::drawHelix( bField , charge, xs, ys, zs , 
                                px, py, pz, ml , 1 ,  0xffffff ,
                                0.0, padLayout.getPlaneExtent()[1]+100. , 
                                gearTPC.getMaxDriftLength()+100. ) ;


      } // track

    } else if( col->getTypeName() == LCIO::MCPARTICLE ){
	
      streamlog_out( DEBUG ) << "  drawing MCParticle collection " << std::endl ;


      double ecalR =  ( Global::GEAR->getEcalBarrelParameters().getExtent()[0] +  
                        Global::GEAR->getEcalEndcapParameters().getExtent()[1]  ) / 2. ;
      
      double ecalZ = std::abs ( Global::GEAR->getEcalEndcapParameters().getExtent()[2] +  
                                Global::GEAR->getEcalEndcapParameters().getExtent()[3]  ) / 2. ;
      
      double hcalR =  ( Global::GEAR->getHcalBarrelParameters().getExtent()[0] +  
                        Global::GEAR->getHcalEndcapParameters().getExtent()[1]  ) / 2. ;
      
      double hcalZ = std::abs ( Global::GEAR->getHcalEndcapParameters().getExtent()[2] +  
                                Global::GEAR->getHcalEndcapParameters().getExtent()[3]  ) / 2. ;
      


      for(int i=0; i<col->getNumberOfElements() ; i++){
	  
        MCParticle* mcp = dynamic_cast<MCParticle*> ( col->getElementAt( i ) ) ;
	  
        float charge = mcp->getCharge (); 
	  
        if( mcp-> getGeneratorStatus() != 1 ) continue ; // stable particles only   

        // 	  if( mcp-> getSimulatorStatus() != 0 ) continue ; // stable particles only   
        //if( mcp->getDaughters().size() > 0  ) continue ;    // stable particles only   
        // FIXME: need definition of stable particles (partons, decays in flight,...)
	  
        if ( mcp->getEnergy() < 0.001 ) continue ;           // ECut ?

        streamlog_out( DEBUG ) << "  drawing MCParticle pdg " 
                               << mcp->getPDG() 
                               << " genstat: " << mcp->getGeneratorStatus() 
                               << std::endl ;


        	  
        double px = mcp->getMomentum()[0]; 
        double py = mcp->getMomentum()[1]; 
        double pz = mcp->getMomentum()[2];


        double x = mcp->getVertex()[0] ;
        double y = mcp->getVertex()[1] ;
        double z = mcp->getVertex()[2] ;	  


        layer = ( layer > -1 ? layer : MCPARTICLE_LAYER ) ;

        //ced_describe_layer( colName.c_str() ,layer);
        MarlinCED::add_layer_description(colName, layer); 


        
        int ml = marker | ( layer << CED_LAYER_SHIFT );
        
        
        if( std::fabs( charge ) > 0.0001  ) { 
	    

          double bField = Global::GEAR->getBField().at(  gear::Vector3D(0,0,0)  ).z() ; 

          streamlog_out( DEBUG ) << "  drawing MCParticle helix for p_t " 
                                 << sqrt(px*px+py*py)
                                 << std::endl ;

          //std::cout<<"Hauke: drawHelix called from cedviewer" << std::endl;
          MarlinCED::drawHelix( bField , charge, x, y, z, 
                                px, py, pz, ml , size , 0x7af774  ,
                                0.0,  padLayout.getPlaneExtent()[1]+100. ,
                                gearTPC.getMaxDriftLength()+100., mcp->id() ) ;	    
	    
        } else { // neutral
	    
          int color  ;
          double r_min = 300 ;
          double z_max ;
          double r_max ;
	    
          if( std::abs( mcp->getPDG() ) == 22 ) {
            color = 0xf9f920;          // photon
            r_max = ecalR ;
            z_max = ecalZ ;
          } else {
            color = 0xb900de  ;        // neutral hadron
            r_max = hcalR ;
            z_max = hcalZ ;  
          } 
	    
          double pt = hypot(px,py);	 
          double p = hypot(pt,pz); 
	    
          double length = ( std::abs( pt/pz) > r_max/z_max ) ?  // hit barrel or endcap ? 
            r_max * p / pt  :  z_max * p / pz  ;
	    
          //hauke hoelbe: add id for picking
          ced_line_ID( r_min*px/p ,  r_min*py/p ,  r_min*pz/p , 
                       length*px/p ,  length*py/p ,  length*pz/p , 
                       ml , size, color, mcp->id() );	 
          
        }
      }
    } else if( col->getTypeName() == LCIO::SIMTRACKERHIT ){

      int color = 0xff00ff ;

      layer = ( layer > -1 ? layer : SIMTRACKERHIT_LAYER ) ;

      //ced_describe_layer( colName.c_str() ,layer);
      MarlinCED::add_layer_description(colName, layer); 


      LCTypedVector<SimTrackerHit> v( col ) ;

//hauke hoelbe
      MarlinCED::drawObjectsWithPosition( v.begin(), v.end() , marker, size , color, layer) ;
//      MarlinCED::drawObjectsWithPositionID(col, v.begin(), v.end() , marker, size , color, layer) ;


    } else if( col->getTypeName() == LCIO::SIMCALORIMETERHIT ){


      int color = 0xff0000 ;

      layer = ( layer > -1 ? layer : SIMCALORIMETERHIT_LAYER ) ;

      //ced_describe_layer( colName.c_str() ,layer);
      MarlinCED::add_layer_description(colName, layer); 
      
      LCTypedVector<SimCalorimeterHit> v( col ) ;
      MarlinCED::drawObjectsWithPosition( v.begin(), v.end() , marker, size , color, layer ) ;

    } else if( col->getTypeName() == LCIO::TRACKERHIT ){

      int color = 0xee0044 ;

      layer = ( layer > -1 ? layer : TRACKERHIT_LAYER ) ;

      //ced_describe_layer( colName.c_str() ,layer);
      MarlinCED::add_layer_description(colName, layer); 


      LCTypedVector<TrackerHit> v( col ) ;
      MarlinCED::drawObjectsWithPosition( v.begin(), v.end() , marker, size , color, layer) ;

    } else if( col->getTypeName() == LCIO::CALORIMETERHIT ){

      int color = 0xee0000 ;
      
      layer = ( layer > -1 ? layer : CALORIMETERHIT_LAYER ) ;

      //ced_describe_layer( colName.c_str() ,layer);
      MarlinCED::add_layer_description(colName, layer); 


      LCTypedVector<CalorimeterHit> v( col ) ;
      MarlinCED::drawObjectsWithPosition( v.begin(), v.end() , marker, size , color, layer ) ;

    }



  } // while

  streamlog_out( MESSAGE ) << " ++++++++ collections shown on layer :   +++++++++++++ " << std::endl ;

  for(unsigned np=0 ; np < nCols ; ++np){

    const std::string & colName = drawParameters[np].ColName ;
//     int size =   drawParameters[np].Size ;
//     int marker = drawParameters[np].Marker ;
    int layer =  drawParameters[np].Layer ;

    streamlog_out( MESSAGE )  << " ++++++++  " << colName <<  "\t  on layer: " << layer << std::endl ;
  }
  streamlog_out( MESSAGE ) << " ++++++++ use shift-[LN] for LN>10  +++++++++++++ " << std::endl ;



  //++++++++++++++++++++++++++++++++++++
  MarlinCED::draw(this) ;
  //++++++++++++++++++++++++++++++++++++
  
  _nEvt ++ ;
}



void CEDViewer::check( LCEvent * evt ) { 
  // nothing to check here - could be used to fill checkplots in reconstruction processor
}

void CEDViewer::printParticle(int id, LCEvent * evt){
  std::cout << "CEDViewer::printParticle id: " << id << std::endl;
} 


void CEDViewer::end(){ 
  
  streamlog_out(DEBUG) << "end() :" << " processed " << _nEvt 
                       << " events in " << _nRun << " runs "
                       << std::endl ;
  
}

#endif
