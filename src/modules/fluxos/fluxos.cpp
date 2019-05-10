
// Copyright 1992: Cornel Beffa and Roland Faeh
// Copyright 2013: Kashif Shaad and Diogo Costa
// Copyright 2019, Diogo Costa

// This program, FLUXOS, is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "fluxos.hpp"

REGISTER_MODULE_CPP(fluxos);

fluxos::fluxos(config_file cfg) :
    module_base("fluxos", parallel::domain, cfg)
{
    depends("p");

    provides("water_level");
}

fluxos::~fluxos()
{

}


void fluxos::initiation(declavar& ds)
{
    
    std::unique_ptr<double[]> zbs1(new double[ds.mx]);   
    double zbsw,zbnw,zbse,zbne,zbsum;
    unsigned int a,iy,ix,ix1,iy1;
    unsigned int nx1,ny1;
    nx1=ds.nx+1;
    ny1=ds.ny+1;

    // INTERPOLATE ELEVATIONS OF THE BOUNDARIES
    for(ix=0;ix<=nx1;ix++){
        zbs1[ix]=(*ds.zb).at(ix,1);
        (*ds.zb).at(ix,0) = (*ds.zb).at(ix,1);
        (*ds.zb).at(ix,ny1) = (*ds.zb).at(ix,ds.ny);
    }
    for(iy=0;iy<=ny1;iy++){
        (*ds.zb).at(0,iy) = (*ds.zb).at(1,iy);
        (*ds.zb).at(nx1,iy) = (*ds.zb).at(ds.nx,iy);
    }

    (*ds.zb).at(0,ny1) = (*ds.zb).at(1,ds.ny);
    (*ds.zb).at(nx1,ny1) = (*ds.zb).at(ds.nx,ds.ny);

    // INITIAL CONDITIONS
    for(iy=1;iy<=ds.ny;iy++)
    {
        for(ix=1;ix<=ds.nx;ix++)
        {
            (*ds.h).at(ix,iy)=std::max((*ds.z).at(ix,iy)-(*ds.zb).at(ix,iy),0.0);
            (*ds.z).at(ix,iy)=(*ds.zb).at(ix,iy)+(*ds.h).at(ix,iy);
            (*ds.p).at(ix,iy)=(*ds.u).at(ix,iy)*(*ds.h).at(ix,iy);
            (*ds.q).at(ix,iy)=(*ds.v).at(ix,iy)*(*ds.h).at(ix,iy);
        }
    }

    arma::mat filedata;
    bool flstatus = filedata.load("initial_conditions.txt",arma::csv_ascii);

    if(flstatus == true)
    {
        for(a=0;a<filedata.col(1).n_elem;a++)
        {
            ix = filedata(a,0);
            iy = filedata(a,1);
            (*ds.h).at(ix,iy) = filedata(a,3);
            (*ds.z).at(ix,iy) = (*ds.zb).at(ix,iy) + filedata(a,3);
            (*ds.u).at(ix,iy) = filedata(a,4);
            (*ds.v).at(ix,iy) = filedata(a,5);
            (*ds.p).at(ix,iy) = filedata(a,6);
            (*ds.q).at(ix,iy) = filedata(a,7);
            (*ds.us).at(ix,iy) = filedata(a,9);
            (*ds.ldry).at(ix,iy) = 0.0f;
        }
    } else
    {
        std::cout << "No initial conditions (file 'initial_conditions.txt not found). All variables set to zero.'" << std::endl;
        for(iy=1;iy<=ds.ny;iy++)
        {
            for(ix=1;ix<=ds.nx;ix++)
            {
                (*ds.h).at(ix,iy) = 0.0f;
                (*ds.z).at(ix,iy) = (*ds.zb).at(ix,iy);
                (*ds.u).at(ix,iy) = 0.0f;
                (*ds.v).at(ix,iy) = 0.0f;
                (*ds.p).at(ix,iy) = 0.0f;
                (*ds.q).at(ix,iy) = 0.0f;
                (*ds.us).at(ix,iy) = 0.0f;
                (*ds.ldry).at(ix,iy) = 1.0f;
            }
        }
    }

    // BOUNDARY VALUES (initial set up)
    for(iy=0;iy<=ny1;iy++)
    {
        (*ds.zb).at(0,iy)=1.5*(*ds.zb).at(1,iy)-.5*(*ds.zb).at(2,iy);
        (*ds.zb).at(nx1,iy)=1.5*(*ds.zb).at(ds.nx,iy)-.5*(*ds.zb).at(ds.nx-1,iy);
        (*ds.z).at(0,iy)=1.5*(*ds.z).at(1,iy)-.5*(*ds.z).at(2,iy);
        (*ds.z).at(nx1,iy)=1.5*(*ds.z).at(ds.nx,iy)-.5*(*ds.z).at(ds.nx-1,iy);
        (*ds.h).at(0,iy)=std::max(0.0,(*ds.z).at(0,iy)-(*ds.zb).at(0,iy));
        (*ds.h).at(nx1,iy)=std::max(0.0,(*ds.z).at(nx1,iy)-(*ds.zb).at(nx1,iy));
        (*ds.p).at(0,iy)=0.0f;
        (*ds.q).at(nx1,iy)=0.0f;
        (*ds.pj).at(0,iy)=0.0f;
    }
    for(ix=0;ix<=nx1;ix++)
    {
        (*ds.zb).at(ix,0)=1.5*(*ds.zb).at(ix,1)-.5*(*ds.zb).at(ix,2);
        (*ds.zb).at(ix,ny1)=1.5*(*ds.zb).at(ix,ds.ny)-.5*(*ds.zb).at(ix,ds.ny-1);
        (*ds.z).at(ix,0)=1.5*(*ds.z).at(ix,1)-.5*(*ds.z).at(ix,2);
        (*ds.z).at(ix,ny1)=1.5*(*ds.z).at(ix,ds.ny)-.5*(*ds.z).at(ix,ds.ny-1);
        (*ds.h).at(ix,0)=std::max(0.0,(*ds.z).at(ix,0)-(*ds.zb).at(ix,0));
        (*ds.h).at(ix,ny1)=std::max(0.0,(*ds.z).at(ix,ny1)-(*ds.zb).at(ix,ny1));
        (*ds.p).at(ix,0)=0.0f;
        (*ds.q).at(ix,ny1)=0.0f;
        (*ds.qj).at(ix,0)=0.0f;
    }
}

void fluxos::solver_dry(declavar& ds, unsigned int ix, unsigned int iy)
{
    
    unsigned int iw,ie,is,in, nxl, nyl;
    double fe1,fe2,fe3,fn1,fn2,fn3,zp,ze,zn,
           he,hn,qe,qp,rp,rn;
    double dze,hme,qme,dzn;
    double hmn,rmn,volrat;
    double ume,vmn,fe2p,fn3p;
    double hp;
    double zbp,zbe,zbn,zbpe,zbpn;
    double dtl;
    float ldw,ldp,lde,lds,ldn;
    double gaccl;
    
    is=iy-1;
    in=iy+1;
    iw=ix-1;
    ie=ix+1;
    
    ldw = (*ds.ldry).at(iw,iy);
    ldp = (*ds.ldry).at(ix,iy);
    lde = (*ds.ldry).at(ie,iy);
    lds = (*ds.ldry).at(ix,is);
    ldn = (*ds.ldry).at(ix,in);
    
    gaccl = ds.gacc;
    nyl = ds.ny;
    nxl = ds.nx;

    // CHECK IF ALL NEIGHBOUR CELLS ARE DRY
    if(ldw==1&&ldp==1&&lde==1&&lds==1&&ldn==1)
    {
        fe1=0.0f;
        fe2=0.0f;
        fe3=0.0f;
        fn1=0.0f;
        fn2=0.0f;
        fn3=0.0f;
        (*ds.dh).at(ix,iy)=0.0f;
        (*ds.dp).at(ix,iy)=0.0f;
        (*ds.dq).at(ix,iy)=0.0f;
        (*ds.pj).at(ix,iy)=0.0f;
        (*ds.qj).at(ix,iy)=0.0f;
    (*ds.p).at(ix,iy)=0.0f;
        (*ds.q).at(ix,iy)=0.0f;   
        return;
    }
    
    // CELL CENTER VALUES
    zbp = (*ds.zb).at(ix,iy);
    zbe = (*ds.zb).at(ie,iy);
    zbn = (*ds.zb).at(ix,in);
    zp=(*ds.z).at(ix,iy);
    ze=(*ds.z).at(ie,iy);
    zn=(*ds.z).at(ix,in);
    hp  = std::max(0.0,(*ds.z).at(ix,iy)-(*ds.z).at(ix,iy));
    he=std::max(0.0,ze-zbe);
    hn=std::max(0.0,zn-zbn);
    qp=(*ds.p).at(ix,iy);
    qe=(*ds.p).at(ie,iy);
    rp=(*ds.q).at(ix,iy);
    rn=(*ds.q).at(ix,in);
       
    // CELL FACE VALUES  
    zbpe=.5*(zbe+zbp);
    zbpn=.5*(zbn+zbp);
    hme=.5*(hp+he);        
    qme=.5*(qp+qe);
    hmn=.5*(hp+hn);  
    rmn=.5*(rp+rn);
    dze=ze-zp;
    dzn=zn-zp;
   
    // TIMESTEP
    dtl = ds.dtfl;
    
    // INITIATION
    fe1=0.0f;
    fe2=0.0f;
    fe3=0.0f;
    fn1=0.0f;
    fn2=0.0f;
    fn3=0.0f;
    
    // CELLS WITH SOME DRY NEIGHBOURS
    if (lde==0.0f)
    {
        hme=std::max(0.0,ze-zbpe);
        if(hme>ds.hdry) 
        {
            if(ze<=zp) 
            {
                fe2p=.5*gaccl*hme*hme;
                fe1=0.0f;
                fe2=fe2p;
                fe3=0.0f;
            }else 
            {
                dze=std::fmin(std::fabs(dze),hme);
                qme=0.296*dze*sqrt(gaccl*dze);      // Ritter solution
                hme=0.444*dze;                      // at cell side           
                volrat=ds.dxy*he/dtl;               // available volume rate per m of cell E [m2/s]
                qme=-std::fmin(qme,volrat);
                ume=qme/hme;                        // from cell center
                fe1=qme;
                fe2=qme*ume + .5*gaccl*hme*hme;
                fe3=0.0f;
            }
        }else 
        {
            fe1=0.0f;
            fe2=0.0f;
            fe3=0.0f;
        }
    }
    if (ldn==0.0f)  
    {   
        hmn=std::max(0.0,zn-zbpn);    
        if(hmn>ds.hdry) 
        {
            if(zn<=zp) 
            {
                fn3p=.5*gaccl*hmn*hmn;
                fn1=0.0f;
                fn2=0.0f;
                fn3=fn3p;
            }else 
            {
            dzn=std::fmin(std::fabs(dzn),hmn);
            rmn=0.296*dzn*sqrt(gaccl*dzn);        // Ritter solution
            hmn=0.444*dzn;                       // at cell side           
            volrat=ds.dxy*hn/dtl;   // available volume rate per m of cell N [m2/s]
            rmn=-std::fmin(rmn,volrat);
            vmn=rmn/hmn;
            fn1=rmn;
            fn2=0.0f;
            fn3=rmn*vmn +.5*gaccl*hmn*hmn;
            }
        }else 
        {
            fn1=0.0f;
            fn2=0.0f;
            fn3=0.0f;
        }
    }
    
    // BOUNDARY CONDITIONS (WEIR DISCHARGE RATE)
    if (iy==1 || iy==nyl)
    {
        fn1=std::min(volrat,sqrt(gaccl)*pow(std::fmax(hp,0.0f),1.5));
    }
    if (ix==1 || ix==nxl)
    {
        fe1=std::min(volrat,sqrt(gaccl)*pow(std::fmax(hp,0.0f),1.5));
    }
    
    // SAVE MASS AND MOMENTUM FLUXES
    (*ds.fn_1).at(ix,iy)=fn1;
    (*ds.fn_2).at(ix,iy)=fn2;
    (*ds.fn_3).at(ix,iy)=fn3;
    (*ds.fe_1).at(ix,iy)=fe1;
    (*ds.fe_2).at(ix,iy)=fe2;
    (*ds.fe_3).at(ix,iy)=fe3;   
} 

void fluxos::solver_wet(declavar& ds, unsigned int ix, unsigned int iy)
{

    unsigned int iw,ie, is,in,inn, nxl, nyl,dx,dy;
    double fe1,fe2,fe3,fn1,fn2,fn3,zw,zp,ze,zs,
        zn,znn,hw,he,hs,hn,hnn,fe2p,fn3p,qe,qp,qn,rw,rp,re,rn;
    double dze,dqe,dre,
        hme,qme,rme,dzn;
    double dqn,drn,hmn,hne,qmn,rmn,volrat,ume,vmn,volpot,cf;
    double cme,cmn,vme,umn,//dzbewr,dzbnsr,
        cnp,cne,cnn,up,un,us0,use0,une,vp,ve,vw,vwn,ven,txye,txyn,fe1c,fe2c,fe3c,fn1c,fn2c,fn3c,dzea,dhea;
    double cc,c1,c2,c3,c1a,c2a,c3a,a11,a12,a21,a22,a31,a32,a33,fe1r,fe2r,fe3r,dzna,dhna,a13,a23,fn1r,fn2r,fn3r;
    double hp ,hp0; 
    double zbw,zbp,zbe,zbs,zbn,zbnn,zbpe,zbpn; //zbpw,zbps
    double dtl, hdryl,gaccl,kspl, nueml, cvdefl; 
    float ldp,lde,ldn;
    bool lroe;

    nxl = ds.nx;
    nyl = ds.ny;
    hdryl = ds.hdry;
    gaccl = ds.gacc;
    kspl = (*ds.ks).at(ix,iy);
    nueml = ds.nuem;
    cvdefl = ds.cvdef;
    
    is=iy-1;
    in=iy+1;
    inn=fmin(iy+2,nyl+1);
    lroe = true;
    iw=ix-1;
    ie=ix+1;
   
    dx  = ds.dxy;
    dy  = ds.dxy;
    
    ldp = (*ds.ldry).at(ix,iy);
    lde = (*ds.ldry).at(ie,iy);
    ldn = (*ds.ldry).at(ix,in);

    // CELL CENTER VALUES
    zbw = (*ds.zb).at(iw,iy);
    zbp = (*ds.zb).at(ix,iy);
    zbe = (*ds.zb).at(ie,iy);
    zbs = (*ds.zb).at(ix,is);
    zbn = (*ds.zb).at(ix,in);
    zbnn= (*ds.zb).at(ix,inn);
    zw=(*ds.z).at(iw,iy);
    zp=(*ds.z).at(ix,iy);
    ze=(*ds.z).at(ie,iy);
    zs=(*ds.z).at(ix,is);
    zn=(*ds.z).at(ix,in);
    znn=(*ds.z).at(ix,inn);
    qp=(*ds.p).at(ix,iy);
    qe=(*ds.p).at(ie,iy);
    qn=(*ds.p).at(ix,in);
    rw=(*ds.q).at(iw,iy);
    rp=(*ds.q).at(ix,iy);
    re=(*ds.q).at(ie,iy);
    rn=(*ds.q).at(ix,in);

    // zbpw=.5*(zbw+zbp);
    zbpe=.5*(zbe+zbp);
    // zbps=.5*(zbs+zbp);
    zbpn=.5*(zbn+zbp);
    hp  = std::max(0.0,(*ds.z).at(ix,iy)-zbp);
    hp0 = std::max(std::max(hdryl,hp),kspl);
    hw=std::max(0.0,zw-zbw);
    he=std::max(0.0,ze-zbe);
    hs=std::max(0.0,zs-zbs);
    hn=std::max(0.0,zn-zbn);
    hnn=std::max(0.0,znn-zbnn);

    // CELL FACE VALUES                    
    hme=.5*(hp+he);          
    qme=.5*(qp+qe);
    rme=.5*(rp+re);
    hmn=.5*(hp+hn);   
    qmn=.5*(qp+qn);
    rmn=.5*(rp+rn);

    dze=ze-zp;
    dqe=qe-qp;
    dre=re-rp;
    dzn=zn-zp;
    drn=rn-rp;
    dqn=qn-qp;
    
    // TIMESTEP
    dtl=ds.dtfl;

    // CELLS WITH SOME DRY NEIGHBOURS     
    if(lde==1.0f) 
    { 
        hme=std::max(0.0,zp-zbpe);
        he=0.0f;;
        qe=0.0f;
        re=0.0f;
        if(hme>hdryl) 
        {
             if(ze>=zp) {
                qme=0.0f;
                rme=0.0f;
            }else 
            {
                dze=fmin(fabs(dze),hme);
                qme=0.296*dze*sqrt(gaccl*dze);    // Ritter solution
                volrat=0.5*dx*hp/dtl;            // available volume rate per m of cell P [m2/s]
                qme=fmin(qme,volrat);
                rme=0.0f;
            }
        }else 
        {
            qme=0.0f;
            rme=0.0f;
            hme=0.0f;            
        }
        lroe= false;
    }
    if(ldn==1.0f) 
    {
        hmn=std::max(0.0,zp-zbpn);
        hn=0.0f;
        qn=0.0f;
        rn=0.0f;
        if(hmn>hdryl) 
        {
            if(zn>=zp) 
            {
                qmn=0.0f;
                rmn=0.0f;
            }else 
            {
                dzn=fmin(fabs(dzn),hmn);
                rmn=0.296*dzn*sqrt(gaccl*dzn);    // Ritter solution
                qmn=0.0f;
                volrat=0.5*dy*hp/dtl;            // available volume rate per m of cell P [m2/s]
                rmn=fmin(rmn,volrat);
            }
        }else 
        {
            qmn=0.0f;
            rmn=0.0f;
            hmn=0.0f;
        }
        lroe=false;
    }

    // CALC TURBULENT STRESS
    cme=sqrt(gaccl*hme);
    cmn=sqrt(gaccl*hmn);
    ume=qme/std::max(hme,hdryl);
    vme=rme/std::max(hme,hdryl);
    umn=qmn/std::max(hmn,hdryl);
    vmn=rmn/std::max(hmn,hdryl);
    
    cnp=cvdefl*(*ds.us).at(ix,iy)*hp+nueml;
    cne=cvdefl*(*ds.us).at(ie,iy)*he+nueml;
    cnn=cvdefl*(*ds.us).at(ix,in)*hn+nueml;
    hne=.5*(cnp+cne)*sqrt(hp*he);
    hnn=.5*(cnp+cnn)*sqrt(hp*hn);

    up=qp/hp0;
    un=qn/std::max(std::max(hn,hdryl),(*ds.ks).at(ix,in));
    us0=(*ds.p).at(ix,is)/std::max(std::max(hs,hdryl),(*ds.ks).at(ix,is));
    use0=(*ds.p).at(ie,is)/std::max(std::max((*ds.h).at(ie,is),hdryl),(*ds.ks).at(ie,is));
    une=(*ds.p).at(ie,in)/std::max(std::max((*ds.h).at(ie,in),hdryl),(*ds.ks).at(ie,in));
    vp=rp/hp0;
    ve=re/std::max(std::max(he,hdryl),(*ds.ks).at(ie,iy));
    vw=rw/std::max(std::max(hw,hdryl),(*ds.ks).at(iw,iy));
    vwn=(*ds.q).at(iw,in)/std::max(std::max((*ds.h).at(iw,in),hdryl),(*ds.ks).at(iw,in));
    ven=(*ds.q).at(ie,in)/std::max(std::max((*ds.h).at(ie,in),hdryl),(*ds.ks).at(ie,in));

    txye=hne*((ve-vp)/fabs(dx)+.25*(un+une-us0-use0)/dy);
    txyn=hnn*((un-up)/fabs(dy)+.25*(ve+ven-vw-vwn)/dx);

    // CALC OF CONVECTION FLUXES
    fe1c=qme;
    fe2c=qme*ume;
    fe3c=qme*vme -txye;
    fn1c=rmn;
    fn2c=rmn*umn -txyn;
    fn3c=rmn*vmn;

    // ROE's DISSIPASSION
    if(lroe) 
    { 
        if(hme>ds.hdry) 
        {
            dzea=fabs(dze);
            if(dzea>0.5*hme) 
            {
                dhea=fabs(he-hp);
                dzea=fmin(dzea,dhea);
                dze=std::copysign(dzea,dze);
            }
            cc=.25/cme;
        }else 
        {
            cc=0.0f;
        }
        
        c1=ume;
        c2=ume+cme;
        c3=ume-cme;
        c1a=fabs(c1);
        c2a=fabs(c2);
        c3a=fabs(c3);
        a11=c2*c3a-c2a*c3;
        a12=c2a-c3a;
        a21=c2*c3*(c3a-c2a);
        a22=c2a*c2-c3a*c3;
        a31=vme*(c2*c3a-2.*cme*c1a-c2a*c3);
        a32=vme*(c2a-c3a);
        a33=2.*cme*c1a;

        fe1r=-(a11*dze+a12*dqe)*cc;
        fe2r=-(a21*dze+a22*dqe)*cc;
        fe3r=-(a31*dze+a32*dqe+a33*dre)*cc;

        if(ldp==0.0f&&hmn>ds.hdry) 
        {
                dzna=fabs(dzn);
                dhna=fabs(hn-hp);
                dzna=fmin(dzna,dhna);
                dzn=std::copysign(dzna,dzn);
                cc=.25/cmn;
        }else 
        {
            cc=0.0f;
        };
        c1=vmn;
        c2=vmn+cmn;
        c3=vmn-cmn;
        c1a=std::fabs(c1);
        c2a=std::fabs(c2);
        c3a=std::fabs(c3);
        a11=c2*c3a-c2a*c3;
        a13=c2a-c3a;
        a21=umn*(c2*c3a-2.*cmn*c1a-c2a*c3);
        a22=2.*cmn*c1a;
        a23=umn*(c2a-c3a);
        a31=c2*c3*(c3a-c2a);
        a33=c2a*c2-c3a*c3;

        fn1r=-(a11*dzn+a13*drn)*cc;
        fn2r=-(a21*dzn+a22*dqn+a23*drn)*cc;
        fn3r=-(a31*dzn+a33*drn)*cc;
    } else
    {
        fe1r=0.0f;
        fe2r=0.0f;
        fe3r=0.0f;
        fn1r=0.0f;
        fn2r=0.0f;
        fn3r=0.0f;
    }
    
    // PRESSURE AT CELL SIDE
    fe2p=.5*gaccl*(hme*hme);
    fn3p=.5*gaccl*(hmn*hmn); 

    // SUM OF ALL FLUXES
    fe1=fe1c+fe1r;
    fe2=fe2c+fe2r+fe2p;
    fe3=fe3c+fe3r;
    fn1=fn1c+fn1r;
    fn2=fn2c+fn2r;
    fn3=fn3c+fn3r+fn3p;
            
    // BOUNDARY CONDITIONS (WEIR DISCHARGE RATE) 
    if (iy==1 || iy==nyl)
    {
        fn1=std::min(volrat,sqrt(gaccl)*pow(std::fmax(hp,0.0f),1.5));
    }
    if (ix==1 || ix==nxl)
    {
        fe1=std::min(volrat,sqrt(gaccl)*pow(std::fmax(hp,0.0f),1.5));
    }

    // CHECK MASS BALANCE (restrict outflow flux to available water)        
    volpot=ds.arbase*hp;    // volume in cell P [m3]
    volrat=volpot/dtl;      // max flux rate 
        
    if(volrat>0.0f) { // cell has water
        if(fe1>0.0f&&fn1>0.0f) 
        {
            if(fe1*dy+fn1*dx>volrat) {
                cf=fn1*dx/(fe1*dy+fn1*dx);
                fe1=(1.-cf)*volrat/dy;            // [m3/s]
                fn1=cf*volrat/dx;
            }
        }else if(fe1>0.0f) 
        {
            fe1=fmin(fe1*dy,volrat)/dy; 
        }else if(fn1>0.0f) 
        {
            fn1=fmin(fn1*dx,volrat)/dx;
        }
    }else // cell has no water 
    {
        fe1=0.0f;
        fn1=0.0f;
    }

    // SAVE MASS AND MOMENTUM FLUXES
    (*ds.fn_1).at(ix,iy)=fn1;
    (*ds.fn_2).at(ix,iy)=fn2;
    (*ds.fn_3).at(ix,iy)=fn3;
    (*ds.fe_1).at(ix,iy)=fe1;
    (*ds.fe_2).at(ix,iy)=fe2;
    (*ds.fe_3).at(ix,iy)=fe3;
    
} 


void fluxos::flow_solver(declavar& ds)
{
           
//-----------------------------------------------------------------------
// Solves shallow water equation for one time-step - Ritter solver
// Pressure term excluded from numerical flux
// Discretized as central difference. 
// Adjustment of source term
// MUSCL-approach with limiter in roe dissipation
// fw1= mass flux per unit width
// fw2= momentum flux per unit width in x-direction
// fw3= momentum flux per unit width in y-direction
//-----------------------------------------------------------------------

    unsigned int ix, iy;
    double hp, dtl;

    dtl = ds.dtfl;
    
    // GET hp AND CHECK IF DRY OR WET
    for(iy=1;iy<=ds.ny;iy++)
    {
        for(ix=1;ix<=ds.nx;ix++)
        {  
            hp=std::max(0.0,(*ds.z).at(ix,iy)-(*ds.zb).at(ix,iy));
            (*ds.h).at(ix,iy) = hp;
            
            if(hp<=ds.hdry)
            {
              (*ds.p).at(ix,iy)=0.0f;
              (*ds.q).at(ix,iy)=0.0f;
              (*ds.us).at(ix,iy)=0.0f;
              (*ds.ldry).at(ix,iy) = 1.0f;;          
            } else
            {
              (*ds.ldry).at(ix,iy) = 0.0f;         
            }
        }
    }
    
    // CALL FLOW SOLVERS (compute mass and momentum fluxes)
     #pragma omp parallel
    {
        #pragma omp for collapse(2)
        for(iy=1;iy<=ds.ny;iy++)
        {
            for(ix=1;ix<=ds.nx;ix++)
            {  
                if((*ds.ldry).at(ix,iy) == 1.0f)
                {
                    solver_dry(ds,ix,iy);
                } else
                {
                    solver_wet(ds,ix,iy);
                }
            }
        }
    }
    
    // CALCULATE TOTAL MASS AND MOMENTUM DERIVATIVE
    for(iy=1;iy<=ds.ny;iy++)
    {
        for(ix=1;ix<=ds.nx;ix++)
        {  
            (*ds.dh).at(ix,iy)=(((*ds.fe_1).at(ix-1,iy)-(*ds.fe_1).at(ix,iy))/ds.dxy +((*ds.fn_1).at(ix,iy-1)-(*ds.fn_1).at(ix,iy))/ds.dxy)*dtl;
            (*ds.dp).at(ix,iy)=(((*ds.fe_2).at(ix-1,iy)-(*ds.fe_2).at(ix,iy))/ds.dxy +((*ds.fn_2).at(ix,iy-1)-(*ds.fn_2).at(ix,iy))/ds.dxy)*dtl;
            (*ds.dq).at(ix,iy)=(((*ds.fe_3).at(ix-1,iy)-(*ds.fe_3).at(ix,iy))/ds.dxy +((*ds.fn_3).at(ix,iy-1)-(*ds.fn_3).at(ix,iy))/ds.dxy)*dtl;
            (*ds.pj).at(ix,iy)=(*ds.fe_1).at(ix,iy)*dtl;
            (*ds.qj).at(ix,iy)=(*ds.fn_1).at(ix,iy)*dtl;
        }
    }

    // CAL NEW VALUES
    for(iy=1;iy<=ds.ny;iy++)
    {
        for(ix=1;ix<=ds.nx;ix++)
        {  
            (*ds.z).at(ix,iy)=(*ds.z).at(ix,iy)+(*ds.dh).at(ix,iy);
            hp=std::max(0.0,(*ds.z).at(ix,iy)-(*ds.zb).at(ix,iy));
            (*ds.h).at(ix,iy)=hp;
            
            if(hp<ds.hdry) 
            {
                (*ds.p).at(ix,iy)= 0.0f;
                (*ds.q).at(ix,iy)= 0.0f;
                (*ds.us).at(ix,iy)= 0.0f;
                (*ds.ldry).at(ix,iy) = 1.0f;
            } else 
            {
                (*ds.p).at(ix,iy)=(*ds.p).at(ix,iy)+(*ds.dp).at(ix,iy);  // numerical flux at cell center
                (*ds.q).at(ix,iy)=(*ds.q).at(ix,iy)+(*ds.dq).at(ix,iy);  // numerical flux at cell center
                (*ds.p).at(ix,iy)=.1*(*ds.pj).at(ix-1,iy)+.8*(*ds.p).at(ix,iy)+.1*(*ds.pj).at(ix,iy); 
                (*ds.q).at(ix,iy)=.1*(*ds.qj).at(ix,iy-1)+.8*(*ds.q).at(ix,iy)+.1*(*ds.qj).at(ix,iy);
                (*ds.ldry).at(ix,iy) = 0.0f;          
            }  
        }
    } 
}

void fluxos::write_results(declavar& ds, int print_tag)
{

    unsigned int iy,ix;
    int a = 0;
    double ux;
    
    std::string tprint = std::to_string(print_tag);
    std::string filext(".txt");
    tprint += filext;

    arma::mat filedataR(ds.nx*ds.ny,10); 
    
    for(iy=1;iy<=ds.ny;iy++)
    {
        for(ix=1;ix<=ds.nx;ix++)
        {
            if ((*ds.h).at(ix,iy)>0.0f)
            {
                ux=sqrt((*ds.u).at(ix,iy) * (*ds.u).at(ix,iy) +
                        (*ds.v).at(ix,iy) * (*ds.v).at(ix,iy));
                filedataR(a,0) = ix;  
                filedataR(a,1) = iy; 
                filedataR(a,2) = (*ds.z).at(ix,iy); 
                filedataR(a,3) = (*ds.z).at(ix,iy) - (*ds.zb).at(ix,iy);
                filedataR(a,4) = (*ds.u).at(ix,iy); 
                filedataR(a,5) = (*ds.v).at(ix,iy); 
                filedataR(a,6) = (*ds.p).at(ix,iy)*ds.dxy;
                filedataR(a,7) = (*ds.q).at(ix,iy)*ds.dxy;
                filedataR(a,8) = ux; 
                filedataR(a,9) = (*ds.us).at(ix,iy); 
                a = a + 1;
            }
        }
    }
   
    arma::mat filedata(std::max(0,a-1),10); 
    filedata = filedataR(arma::span(0,std::max(0,a-1)),arma::span(0,9));
    
    bool flstatus =  filedata.save(tprint,arma::csv_ascii);
   
    if(flstatus == true) 
    {
        std::cout << "Result " + tprint + " saved"  << std::endl;
    } else
    {
        std::cout << "Problem when saving the results:" + tprint << std::endl;
    }


}
    

void fluxos::run(mesh& domain) {

    float print_next;
    size_t ix, iy;
    double c0, v0, u0, hp, hpall;


    // SAVE INITIAL STATUS IN RESULTS (t = 0)
    print_next = 0.0f;
    write_results(ds, std::round(print_next));

    print_next = print_next + print_step;

    //reset
    ds.tim = 0;
    forcing->zeros();
    // setup for the forcing
    for (size_t i = 0; i < domain->size_faces(); i++)
    {
        auto face = domain->face(i);
        double precip = (*face)["p"]/1000./global_param->dt();

        auto d = face->get_module_data<data>(ID);
        auto& idx = d->rastercells;
        if( d->rastercells.n_elem != 0)
          forcing->elem(idx) = arma::ones<arma::vec>(idx.n_elem)*precip;

//        (*ds.z)(idx) = (*ds.z)(idx) + precip*ds.dtfl;
//        (*ds.h)(idx) = (*ds.z)(idx)-(*ds.zb)(idx);

//            (*ds.h)(idx) = arma::max((*ds.z)(idx)-(*ds.zb)(idx),0.0);


    }


    // TIME LOOP
    while (ds.tim <= ds.ntim)
    {
        ds.dtfl = 9.e10;
        hpall = 0.0f;

        // SPACE LOOP
        for (iy = 1; iy <= ds.ny; iy++) {
            for (ix = 1; ix <= ds.nx; ix++) {
                hp = (*ds.h).at(ix, iy);
                if (hp > ds.hdry) {
                    (*ds.ldry).at(ix, iy) = 0.0f;
                    hp = std::fmax((*ds.h).at(ix, iy), ds.hdry);
                    hpall = std::fmax(hpall, (*ds.h).at(ix, iy));
                    c0 = sqrt(ds.gacc * (*ds.h).at(ix, iy));
                    u0 = std::fmax(.000001, fabs((*ds.p).at(ix, iy) / hp));
                    v0 = std::fmax(.000001, fabs((*ds.q).at(ix, iy) / hp));
                    ds.dtfl = fmin(fmin(ds.cfl * ds.dxy / (u0 + c0), ds.cfl * ds.dxy / (v0 + c0)), ds.dtfl);

                } else {
                    (*ds.ldry).at(ix, iy) = 1.0f;
                }
                ds.dtfl = fmin(print_next - ds.tim, ds.dtfl);
            }
        }

        if(ds.tim == 0. || (*ds.h).max() <= 0.01)
            ds.dtfl = 1;
//        else
//            LOG_DEBUG << ds.dtfl;

        LOG_DEBUG << ds.tim;
        ds.tim = ds.tim + ds.dtfl;

        for (iy = 1; iy <= ds.ny; iy++) {
            for (ix = 1; ix <= ds.nx; ix++) {
                if ((*ds.zb).at(ix,iy)!=9999){
                    (*ds.z).at(ix,iy) = (*ds.z).at(ix,iy) + forcing->at(ix,iy)*ds.dtfl;
                    (*ds.h).at(ix,iy)=std::max((*ds.z).at(ix,iy)-(*ds.zb).at(ix,iy),0.0);
                }
            }
        }


//        //figure out which raster cells go with what triangle
//        for (size_t i = 0; i < domain->size_faces(); i++)
//        {
//            auto face = domain->face(i);
//            double precip = (*face)["p"]/1000./global_param->dt();
//
//            auto d = face->get_module_data<data>(ID);
//            auto& idx = d->rastercells;
//
//
//
//            (*ds.z)(idx) = (*ds.z)(idx) + precipc;
//            (*ds.h)(idx) = (*ds.z)(idx)-(*ds.zb)(idx);
//
////            (*ds.h)(idx) = arma::max((*ds.z)(idx)-(*ds.zb)(idx),0.0);
//
//
//        }


        // FLOW SOLVERS
        if (hpall != 0) {
            flow_solver(ds);
        }

        // PRINT RESULTS
        if (ds.tim >= print_next) {
            write_results(ds, std::round(print_next));
            print_next = print_next + print_step;
        }

    }

    //figure out which raster cells go with what triangle
    for (size_t i = 0; i < domain->size_faces(); i++)
    {
        auto face = domain->face(i);
        auto d = face->get_module_data<data>(ID);

        if( d->rastercells.n_elem != 0)
          (*face)["water_level"] = arma::mean((*ds.h).elem(d->rastercells));
    }

}

void fluxos::init(mesh& domain)
{
    arma::mat filedata;
    bool flstatus =  filedata.load( cfg.get("model_geo","model_geo.txt"),arma::raw_ascii);

    if(!flstatus) {
        BOOST_THROW_EXCEPTION(module_error() << errstr_info("problem with loading 'modelgeo.txt'"));
    }

    filedata.replace(-9999.0,9999.0);



    size_t nxl = filedata.n_cols;
    size_t nyl = filedata.n_rows;

    LOG_DEBUG << "Read in structured mesh of size " + std::to_string(nxl) + " " + std::to_string(nyl);

    tri_mapping = std::unique_ptr<arma::Mat<double>>( new  arma::mat);
    flstatus = tri_mapping->load( cfg.get("cell_mapping","cellmapping.txt"),arma::raw_ascii);
    if(!flstatus)
    {
        BOOST_THROW_EXCEPTION(module_error() << errstr_info("problem with loading 'cellidmapping.txt'"));
    }


    LOG_DEBUG << "Read in cellid mapping of size " + std::to_string(nxl) + " " + std::to_string(nyl);

    if(tri_mapping->n_cols != filedata.n_cols ||
        tri_mapping->n_rows != filedata.n_rows)
    {
        BOOST_THROW_EXCEPTION(module_error() << errstr_info("Inconsistent dimensions in both structured meshes"));
    }

    if(tri_mapping->max() >domain->size_faces() )
      BOOST_THROW_EXCEPTION(module_error() << errstr_info("Max mapping is out of bounds"));

    //figure out which raster cells go with what triangle
    for (size_t i = 0; i < domain->size_faces(); i++)
    {
        auto face = domain->face(i);
        auto d = face->make_module_data<data>(ID);

        d->rastercells = arma::find( *tri_mapping == face->cell_global_id );

        if( d->rastercells.n_elem == 0)
        {
//          LOG_DEBUG << face->cell_global_id;
//          LOG_DEBUG << i;
//            BOOST_THROW_EXCEPTION(module_error() << errstr_info("Triangle does not map to any rasters"));
        }
    }

    ds.mx = nxl+2;
    ds.my = nyl+2;
    size_t mx = ds.mx;
    size_t my = ds.my;

    forcing = std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx-2,my-2));

    ds.z= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.zb= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.h= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.u= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.v= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.p= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.q= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.pj= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.qj= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.us= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.dh= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.dp = std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.dq= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    //sbmx= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    //sbmy= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    //cfri= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.ks= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));

//  f_1= mass flux per unit width
//  f_2= momentum flux per unit width in x-direction
//  f_3= momentum flux per unit width in y-direction
    ds.fe_1= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.fe_2= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.fe_3= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.fn_1= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.fn_2= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.fn_3= std::unique_ptr<arma::Mat<double>>( new  arma::mat(mx,my));
    ds.ldry= std::unique_ptr<arma::Mat<float>>( new  arma::fmat(mx,my));

    // input/read data
    ds.cfl = 1; // Courant condition
    ds.dxy = cfg.get("dxy",3); // grid size (structure grid) - it will actually come from DEM
//    ds.ntim = cfg.get("ntim",3000000);// maximum time step (seconds)
    //kapa = -2.    // /  -2=1.Ord ; -1=2.Ord   // KOMISCH, DASS REAL/INTEGER ->schauen bei Rolands Dateien
    ds.arbase = ds.dxy * ds.dxy;
    //betas = 2. // Chezy (parameter)
    //ksfix = 0.2 // Chezy (rougness) -> NEEDs to be converted into a vector with data for all cells
    ds.cvdef = 0.07; // for turbulent stress calc
    ds.nuem = 1.2e-6; // molecular viscosity (for turbulent stress calc)
    print_step = cfg.get("print_step",3600); // in seconds

    ds.nx = ds.mx - 2;
    ds.ny = ds.my - 2;

    //comfirm these are deep copies and behave as expected
    (*ds.zb) = filedata;
    (*ds.ks) = 0.01;
//    unsigned int iy,ix,a;
//        for(a=0;a<filedata.col(1).n_elem;a++){
//            ix = filedata(a,0);
//            iy = filedata(a,1);
//            (*ds.zb).at(ix,iy) = filedata(a,2);
//            (*ds.ks).at(ix,iy) = filedata(a,3);
//        }

    initiation(ds);

    // INITIATION
    ds.hdry = (*ds.ks).at(1,1);  // temporary but basically saying that nothing will move until it reaches roughness height
    ds.tim = 0.0f;

    ds.ntim = global_param->dt(); // dt = model timestep so don't step past that
}