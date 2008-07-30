#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Leo Zayas
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import gtk
import gtk.gdk
import math
import cairo
import time
import copy
import emtraffic
import gobject
import os

import MSD

from MSD.MSDConsumManager import traffic
from MSD.MSDConsumManager import traffic_roaming

TRAFFIC_HISTORY_GRAPH_UPDATE_TIME_MS = 2*60*1000

class Rect:
	def __init__(self):
		self.left=self.top=self.right=self.bottom=0
	
	def __init__(self, left, top, right, bottom):
		self.left=left
		self.top=top
		self.right=right
		self.bottom=bottom

class MSDTrafficHistoryGraph(gtk.DrawingArea):
	def __init__(self):
		gtk.DrawingArea.__init__(self)

		self.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))

		self.connect("expose-event", self.draw)
		
		self.vis_cols=6
		self.pos=0
		self.values=[]
		
		self.reload_data()
		self.timeout=gobject.timeout_add(TRAFFIC_HISTORY_GRAPH_UPDATE_TIME_MS, self.on_timeout)				

	def invalidate(self):
		if self.window:
			alloc = self.get_allocation()
			rect = gtk.gdk.Rectangle(0, 0, alloc.width, alloc.height)
			self.window.invalidate_rect(rect, True)
			self.window.process_updates(True)	
			
	def set_values(self, values): # list is [[[val_local, val_roaming],tag], ...]
		last=len(self.values)==0 or self.pos==max(0, len(self.values)-self.vis_cols)
		self.values=values
		if last:
			self.pos=max(0, len(self.values)-self.vis_cols)
		if self.pos>len(self.values)-self.vis_cols:
			self.pos=len(self.values)-self.vis_cols
		if self.pos<0:
			self.pos=0
		self.invalidate()
		
	def get_visible_columns(self):
		return self.vis_cols
		
	def get_num_columns(self):
		return len(self.values)
		
	def get_pos(self):
		return self.pos
			
	def set_pos(self, pos):
		self.pos=pos
		if self.pos>len(self.values)-self.vis_cols:
			self.pos=len(self.values)-self.vis_cols
		if self.pos<0:
			self.pos=0
		self.invalidate()

	def fill_rect(self, ctx, rect):
		ctx.move_to(rect.left,rect.top)
		ctx.line_to(rect.right,rect.top)
		ctx.line_to(rect.right,rect.bottom)
		ctx.line_to(rect.left,rect.bottom)
		ctx.line_to(rect.left,rect.top)
		ctx.fill()
		ctx.stroke()
		
	def draw(self, widget, event):
		ctx=widget.window.cairo_create()
		ctx.rectangle(event.area.x, event.area.y,
			event.area.width, event.area.height)
		ctx.clip()
		
		rc=self.get_allocation()

		max_data=0
		
		for d in xrange(len(self.values)):
			if d>=self.pos and d-self.pos<self.vis_cols: # scale graph to visible values
				for d2 in xrange(2):
					max_data=max(max_data, self.values[d][0][d2])

		barras=Rect(50,15,rc.width-10,rc.height-50)			
	
		colors=[
			[ [[0.4,0.4,0.4],[0.4,0.4,0.6],[0.4,0.4,1.0]], [[0.8,0.2,0.2],[0.8,0.2,0.2],[1.0,0.313,0.313]] ],
			[ [[0.0,0.0,0.2],[0.0,0.0,0.4],[0.2,0.2,1.0]], [[0.6,0.0,0.0],[0.6,0.0,0.0],[0.8,0.0,0.0]] ]
		]
					
		for b in xrange(self.vis_cols):
			if self.pos+b>=len(self.values):
				continue;				
			x1=barras.left+2+(barras.right-(barras.left+2))*b/self.vis_cols
			x2=barras.left+2+(barras.right-(barras.left+2))*(b+1)/self.vis_cols
			w=x2-x1
			centerx=(x1+x2)/2
			x1+=w*10/100
			x2-=w*10/100
			w=x2-x1
			
			for d in xrange(2):
				l=x1+w*d/2;
				r=x1+w*(d+1)/2 -1;
				if r<l+5:
					r=l+5;					
				data=self.values[self.pos+b][0][d]
				if max_data!=0:
					h=data*(barras.bottom-barras.top)/max_data;
				else:
					h=0

				if self.pos+b==len(self.values)-1:
					col=1
				else:
					col=0
				color_border=colors[col][d][0]
				color_bar1=colors[col][d][1]
				color_bar2=colors[col][d][2]
				
				re=Rect(l,barras.bottom-h,r,barras.bottom)
				if re.bottom>=re.top+5:
					for x in xrange(l, r-2, 2):
						re2=Rect(x,re.top+2,x+2,re.bottom-2)
						rr=color_bar1[0]+(color_bar2[0]-color_bar1[0])*(x-l)/(r-l)
						gg=color_bar1[1]+(color_bar2[1]-color_bar1[1])*(x-l)/(r-l)
						bb=color_bar1[2]+(color_bar2[2]-color_bar1[2])*(x-l)/(r-l)
						ctx.set_source_rgb(rr,gg,bb)
						self.fill_rect(ctx, re2)
						
				ctx.set_source_rgb(color_border[0],color_border[1],color_border[2])
				self.fill_rect(ctx, Rect(re.left,re.top,re.left+2,re.bottom))
				if re.top+2<=re.bottom:
					self.fill_rect(ctx, Rect(re.left,re.top,re.right,re.top+2))
				self.fill_rect(ctx, Rect(re.right-2,re.top,re.right,re.bottom))
				if re.bottom-2>=re.top:
			 		self.fill_rect(ctx, Rect(re.left,re.bottom-2,re.right,re.bottom))
			 		
			 	text="%i" % int(data)
			 	ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, 
			 		cairo.FONT_WEIGHT_NORMAL)
			 	ctx.set_font_size(0.75*10.0)
			 	extent=ctx.text_extents(text)			 	
			 	fextent=ctx.font_extents()			 	
			 	ctx.set_source_rgb(0,0,0)
			 	ctx.move_to((l+r)/2-extent[2]/2,re.bottom+1+fextent[0]) # ascent
			 	ctx.show_text(text)
			 	ctx.stroke()

		 	text="0000"
		 	ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, 
		 		cairo.FONT_WEIGHT_NORMAL)
		 	ctx.set_font_size(0.75*10.0)
		 	fextent0=ctx.font_extents()			 	
			 	
		 	text=self.values[self.pos+b][1]
		 	ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, 
		 		cairo.FONT_WEIGHT_BOLD)
		 	ctx.set_font_size(10.0*1.25)
		 	extent=ctx.text_extents(text)			 	
		 	fextent=ctx.font_extents()			 	
		 	ctx.set_source_rgb(0,0,0)
		 	ctx.move_to((x1+x2)/2-extent[2]/2, barras.bottom+fextent0[2]+fextent[0]) # height + ascent
		 	ctx.show_text(text)
		 	ctx.stroke()
		
	 	ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, 
	 		cairo.FONT_WEIGHT_NORMAL)
	 	ctx.set_font_size(0.75*10.0)
		for s in xrange(1,5):
			y=barras.bottom+(barras.top-barras.bottom)*s/4;
			text="%i" % int(max_data*s/4)
			extent=ctx.text_extents(text)
			fextent=ctx.font_extents()
			ctx.move_to(barras.left-extent[2]-2,y-extent[3]/2+fextent[0])
			ctx.show_text(text)
			ctx.stroke()
			
		rc=Rect(0,0,rc.width,rc.height)
		
	 	ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, 
	 		cairo.FONT_WEIGHT_NORMAL)
	 	ctx.set_font_size(10.0)
	 	extent=ctx.font_extents()
	 	fascent=extent[0]
	 	fdescent=extent[1]
	 	fheight=extent[2]	 	
		
		x=rc.left+7
		ctx.set_source_rgb(0,0,0.2)
		self.fill_rect(ctx, Rect(x,rc.bottom-10-fdescent,x+30,rc.bottom-10+7-fdescent))
		ctx.set_source_rgb(0.2,0.2,1.0)
		self.fill_rect(ctx, Rect(x+2,rc.bottom-10+2-fdescent,x+30-2,rc.bottom-10+7-2-fdescent))
		
		x+=33
		text=_(u"Red movistar")
		ext=ctx.text_extents(text)
		ctx.move_to(x,rc.bottom-3-fdescent)
		ctx.set_source_rgb(0,0,0)
		ctx.show_text(text)
		ctx.stroke()
		
		x+=ext[2]+10;
		ctx.set_source_rgb(0.2,0,0)
		self.fill_rect(ctx, Rect(x,rc.bottom-10-fdescent,x+30,rc.bottom-10+7-fdescent))
		ctx.set_source_rgb(0.8,0,0)
		self.fill_rect(ctx, Rect(x+2,rc.bottom-10+2-fdescent,x+30-2,rc.bottom-10+7-2-fdescent))
		
		x+=33
		text=_(u"Roaming")
		ext=ctx.text_extents(text)
		ctx.move_to(x,rc.bottom-3-fdescent)
		ctx.set_source_rgb(0,0,0)
		ctx.show_text(text)
		ctx.stroke()

		ctx.set_source_rgb(0.4,0.4,0.4)		
		self.fill_rect(ctx, Rect(barras.left,barras.top,barras.left+2,barras.bottom))
		self.fill_rect(ctx, Rect(barras.left-9,barras.bottom-2,barras.right,barras.bottom))
				
		return False

	def on_timeout(self):
		self.reload_data()
		self.timeout=gobject.timeout_add(TRAFFIC_HISTORY_GRAPH_UPDATE_TIME_MS, self.on_timeout)

	def reload_data(self):
		dun=emtraffic.get_traffic_history("DUN")
		dunr=emtraffic.get_traffic_history("DUNR")
		pend=traffic.get_pending_traffic_history()
		pendr=traffic_roaming.get_pending_traffic_history()
		
		if len(dun)>0:
			minm=dun[0][0][1]*12+dun[0][0][0]-1;
			maxm=dun[len(dun)-1][0][1]*12+dun[len(dun)-1][0][0]-1;
		if len(dunr)>0:
			if len(dun)==0:
				minm=dunr[0][0][1]*12+dunr[0][0][0]-1;
				maxm=dunr[len(dunr)-1][0][1]*12+dunr[len(dunr)-1][0][0]-1;
			else:
				minm=min(minm,dunr[0][0][1]*12+dunr[0][0][0]-1);
				maxm=max(maxm,dunr[len(dunr)-1][0][1]*12+dunr[len(dunr)-1][0][0]-1);
		if len(dun)==0 and len(dunr)==0:
			minm=pend[0][1]*12+pend[0][0]-1;
			maxm=pend[0][1]*12+pend[0][0]-1;
		else:
			minm=min(minm,pend[0][1]*12+pend[0][0]-1);
			maxm=max(maxm,pend[0][1]*12+pend[0][0]-1);
		minm=min(minm,pendr[0][1]*12+pendr[0][0]-1);
		maxm=max(maxm,pendr[0][1]*12+pendr[0][0]-1);
			
		print minm
		print maxm
		
		datas=[]
		for h in xrange(minm, maxm+1):
			m=h%12
			a=h/12
			data=[[0,0],""]
			for d in xrange(0, len(dun)):
				if dun[d][0][1]==a and dun[d][0][0]-1==m:
					data[0][0]+=dun[d][1][0]+dun[d][1][1]
					break
			for d in xrange(0, len(dunr)):
				if dunr[d][0][1]==a and dunr[d][0][0]-1==m:
					data[0][1]+=dunr[d][1][0]+dunr[d][1][1]
					break
			if pend[0][1]==a and pend[0][0]-1==m:
				data[0][0]+=pend[1][0]+pend[1][1]
			if pendr[0][1]==a and pendr[0][0]-1==m:
				data[0][1]+=pendr[1][0]+pendr[1][1]
				
			months=[_(u"ene"),_(u"feb"),_(u"mar"),_(u"abr"),("may"),_(u"jun"),_(u"jul"),_(u"ago"),_(u"sep"),_(u"oct"),_(u"nov"),_(u"dic")]
			month=months[m]
			label="%s'%02i" % (month, a%100)
			data[0][0]/=1024*1024
			data[0][1]/=1024*1024			
			data[1]=label
			datas=datas+[data]

		self.set_values(datas)
		

