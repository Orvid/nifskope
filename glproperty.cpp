/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "glproperty.h"
#include "glcontroller.h"
#include "glscene.h"

Property * Property::create( Scene * scene, const NifModel * nif, const QModelIndex & index )
{
	Property * property = 0;
	
	if ( nif->inherits( index, "AProperty" ) )
	{
		QString name = nif->itemName( index );
		if ( name == "NiAlphaProperty" )
			property = new AlphaProperty( scene, index );
		else if ( name == "NiZBufferProperty" )
			property = new ZBufferProperty( scene, index );
		else if ( name == "NiTexturingProperty" )
			property = new TexturingProperty( scene, index );
		else if ( name == "NiMaterialProperty" )
			property = new MaterialProperty( scene, index );
		else if ( name == "NiSpecularProperty" )
			property = new SpecularProperty( scene, index );
		else if ( name == "NiWireframeProperty" )
			property = new WireframeProperty( scene, index );
		else if ( name == "NiVertexColorProperty" )
			property = new VertexColorProperty( scene, index );
		else if ( name == "NiStencilProperty" )
			property = new StencilProperty( scene, index );
	}
	
	if ( property )
		property->update( nif, index );
	
	return property;
}

PropertyList::PropertyList()
{
}

PropertyList::PropertyList( const PropertyList & other )
{
	operator=( other );
}

PropertyList::~PropertyList()
{
	clear();
}

void PropertyList::clear()
{
	foreach( Property * p, properties )
		del( p );
}

PropertyList & PropertyList::operator=( const PropertyList & other )
{
	clear();
	foreach ( Property * p, other.list() )
		add( p );
	return *this;
}

void PropertyList::add( Property * p )
{
	if ( p && ! properties.contains( p ) )
	{
		++ p->ref;
		properties.append( p );
	}
}

void PropertyList::del( Property * p )
{
	if ( properties.contains( p ) )
	{
		int cnt = properties.removeAll( p );
		
		if ( p->ref <= cnt )
		{
			delete p;
		}
		else
			p->ref -= cnt;
	}
}

Property * PropertyList::get( const QModelIndex & index ) const
{
	foreach ( Property * p, properties )
	{
		if ( p->index().isValid() && p->index() == index )
			return p;
	}
	return 0;
}

void PropertyList::validate()
{
	QList<Property *> rem;
	foreach ( Property * p, properties )
	{
		if ( ! p->isValid() )
			rem.append( p );
	}
	foreach ( Property * p, rem )
	{
		del( p );
	}
}

void AlphaProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	
	if ( iBlock.isValid() && iBlock == block )
	{
		unsigned short flags = nif->get<int>( iBlock, "Flags" );
		
		alphaBlend = flags & 1;
		
		static const GLenum blendMap[16] = {
			GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
			GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE, GL_ONE,
			GL_ONE, GL_ONE, GL_ONE, GL_ONE
		};
		
		alphaSrc = blendMap[ ( flags >> 1 ) & 0x0f ];
		alphaDst = blendMap[ ( flags >> 5 ) & 0x0f ];
		
		static const GLenum testMap[8] = {
			GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
		};
		
		alphaTest = flags & ( 1 << 9 );
		alphaFunc = testMap[ ( flags >> 10 ) & 0x7 ];
		alphaThreshold = nif->get<int>( iBlock, "Threshold" ) / 255.0;
		
		alphaSort = ( flags & 0x2000 ) == 0;
	}
}

void glProperty( AlphaProperty * p )
{
	if ( p && p->alphaBlend && p->scene->blending )
	{
		glEnable( GL_BLEND );
		glBlendFunc( p->alphaSrc, p->alphaDst );
	}
	else
		glDisable( GL_BLEND );
	
	if ( p && p->alphaTest && p->scene->blending )
	{
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( p->alphaFunc, p->alphaThreshold );
	}
	else
		glDisable( GL_ALPHA_TEST );
}

void ZBufferProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	
	if ( iBlock.isValid() && iBlock == block )
	{
		int flags = nif->get<int>( iBlock, "Flags" );
		depthTest = flags & 1;
		depthMask = flags & 2;
		if ( nif->checkVersion( 0x10000001, 0 ) )
		{
			static const GLenum depthMap[8] = {
				GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
			};
			depthFunc = depthMap[ nif->get<int>( iBlock, "Function" ) & 0x07 ];
		}
		else
			depthFunc = GL_LEQUAL;
	}
}

void glProperty( ZBufferProperty * p )
{
	if ( p ) 
	{
		if ( p->depthTest )
		{
			glEnable( GL_DEPTH_TEST );
			glDepthFunc( p->depthFunc );
		}
		else
			glDisable( GL_DEPTH_TEST );
		
		glDepthMask( p->depthMask ? GL_TRUE : GL_FALSE );
	}
	else
	{
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
	}
}

void TexturingProperty::update( const NifModel * nif, const QModelIndex & property )
{
	Property::update( nif, property );
	
	if ( iBlock.isValid() && iBlock == property )
	{
		static const char * texnames[8] = { "Base Texture", "Dark Texture", "Detail Texture", "Gloss Texture", "Glow Texture", "Bump Texture", "Decal0 Texture", "Decal1 Texture" };
		for ( int t = 0; t < 8; t++ )
		{
			QModelIndex iTex = nif->getIndex( property, texnames[t] );
			if ( iTex.isValid() )
				iTex = nif->getIndex( iTex, "Texture Data" );
			if ( iTex.isValid() )
			{
				textures[t].iSource = nif->getBlock( nif->getLink( iTex, "Source" ), "NiSourceTexture" );
				textures[t].coordset = nif->get<int>( iTex, "Texture Set" );
				switch ( nif->get<int>( iTex, "Filter Mode" ) )
				{
					case 0:		textures[t].filter = GL_NEAREST;		break;
					case 1:		textures[t].filter = GL_LINEAR;		break;
					case 2:		textures[t].filter = GL_NEAREST_MIPMAP_NEAREST;		break;
					case 3:		textures[t].filter = GL_LINEAR_MIPMAP_NEAREST;		break;
					case 4:		textures[t].filter = GL_NEAREST_MIPMAP_LINEAR;		break;
					case 5:		textures[t].filter = GL_LINEAR_MIPMAP_LINEAR;		break;
					default:	textures[t].filter = GL_LINEAR;		break;
				}
				switch ( nif->get<int>( iTex, "Clamp Mode" ) )
				{
					case 0:		textures[t].wrapS = GL_CLAMP;	textures[t].wrapT = GL_CLAMP;	break;
					case 1:		textures[t].wrapS = GL_CLAMP;	textures[t].wrapT = GL_REPEAT;	break;
					case 2:		textures[t].wrapS = GL_REPEAT;	textures[t].wrapT = GL_CLAMP;	break;
					default:	textures[t].wrapS = GL_REPEAT;	textures[t].wrapT = GL_REPEAT;	break;
				}
				
				textures[t].hasTransform = nif->get<int>( iTex, "Has Texture Transform" );
				if ( textures[t].hasTransform )
				{
					textures[t].translation = nif->get<Vector2>( iTex, "Translation" );
					textures[t].tiling = nif->get<Vector2>( iTex, "Tiling" );
					textures[t].rotation = nif->get<float>( iTex, "W Rotation" );
					textures[t].center = nif->get<Vector2>( iTex, "Center Offset" );
				}
				else
				{
					textures[t].translation = Vector2();
					textures[t].tiling = Vector2( 1.0, 1.0 );
					textures[t].rotation = 0.0;
					textures[t].center = Vector2( 0.5, 0.5 );
				}
			}
			else
			{
				textures[t].iSource = QModelIndex();
			}
		}
	}
}

class TexFlipController : public Controller
{
public:
	TexFlipController( TexturingProperty * prop, const QModelIndex & index )
		: Controller( index ), target( prop ), flipDelta( 0 ), flipSlot( 0 ) {}
	
	void update( float time )
	{
		const NifModel * nif = static_cast<const NifModel *>( iSources.model() );
		if ( ! ( target && flags.controller.active && iSources.isValid() && nif && flipDelta > 0 && flipSlot == 0 ) )
			return;
		
		int m = nif->rowCount( iSources );
		if ( m == 0 )	return;
		int r = ( (int) ( ctrlTime( time ) / flipDelta ) ) % m;
		
		target->textures[flipSlot & 7 ].iSource = nif->getBlock( nif->getLink( iSources.child( r, 0 ) ), "NiSourceTexture" );
	}

	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			flipDelta = nif->get<float>( index, "Delta" );
			flipSlot = nif->get<int>( index, "Texture Slot" );
			
			iSources = nif->getIndex( nif->getIndex( index, "Sources" ), "Indices" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<TexturingProperty> target;
	
	float	flipDelta;
	int		flipSlot;
	
	QPersistentModelIndex iSources;
};

class TexTransController : public Controller
{
public:
	TexTransController( TexturingProperty * prop, const QModelIndex & index )
		: Controller( index ), target( prop ), texSlot( 0 ), texOP( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( target && flags.controller.active ) )
			return;
		
		TexturingProperty::TexDesc * tex = & target->textures[ texSlot & 7 ];
		
		float val;
		if ( interpolate( val, iValues, ctrlTime( time ), lX ) )
		{
			switch ( texOP )
			{
				case 0:
					tex->translation[0] = val;
					break;
				case 1:
					tex->translation[1] = val;
					break;
				default:
					tex->rotation = val;
					break;
			}
		}
	}

	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			iValues = nif->getIndex( iData, "Data" );
			texSlot = nif->get<int>( index, "Texture Slot" );
			texOP = nif->get<int>( index, "Operation" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<TexturingProperty> target;
	
	QPersistentModelIndex iValues;
	
	int		texSlot;
	int		texOP;
	
	int		lX;
};

void TexturingProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiFlipController" )
	{
		Controller * ctrl = new TexFlipController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( nif->itemName( iController ) == "NiTextureTransformController" )
	{
		Controller * ctrl = new TexTransController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

void glProperty( TexturingProperty * p )
{
	GLTex * tex;
	if ( p && p->scene->texturing && ( tex = p->scene->bindTexture( p->textures[0].iSource ) ) )
	{
		glEnable( GL_TEXTURE_2D );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, p->textures[0].wrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, p->textures[0].wrapT );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		if ( p->textures[0].hasTransform )
		{
			glTranslatef( - p->textures[0].center[0], - p->textures[0].center[1], 0 );
			glRotatef( p->textures[0].rotation, 0, 0, 1 );
			glTranslatef( p->textures[0].center[0], p->textures[0].center[1], 0 );
			glScalef( p->textures[0].tiling[0], p->textures[0].tiling[1], 1 );
			glTranslatef( p->textures[0].translation[0], p->textures[0].translation[1], 0 );
		}
		glMatrixMode( GL_MODELVIEW );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}
}

void MaterialProperty::update( const NifModel * nif, const QModelIndex & index )
{
	Property::update( nif, index );
	
	if ( iBlock.isValid() && iBlock == index )
	{
		alpha = nif->get<float>( index, "Alpha" );
		if ( alpha < 0.0 ) alpha = 0.0;
		if ( alpha > 1.0 ) alpha = 1.0;
		
		ambient = Color4( nif->get<Color3>( index, "Ambient Color" ) );
		diffuse = Color4( nif->get<Color3>( index, "Diffuse Color" ) );
		specular = Color4( nif->get<Color3>( index, "Specular Color" ) );
		emissive = Color4( nif->get<Color3>( index, "Emissive Color" ) );
		
		shininess = nif->get<float>( index, "Glossiness" );
	}
}

class AlphaController : public Controller
{
public:
	AlphaController( MaterialProperty * prop, const QModelIndex & index )
			: Controller( index ), target( prop ), lAlpha( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		interpolate( target->alpha, iAlpha, ctrlTime( time ), lAlpha );
		
		if ( target->alpha < 0 )
			target->alpha = 0;
		if ( target->alpha > 1 )
			target->alpha = 1;
	}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			iAlpha = nif->getIndex( iData, "Data" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<MaterialProperty> target;
	
	QPersistentModelIndex iAlpha;
	
	int lAlpha;
};

void MaterialProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiAlphaController" )
	{
		Controller * ctrl = new AlphaController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

void glProperty( MaterialProperty * p, SpecularProperty * s )
{
	if ( p )
	{
		glMaterial( GL_FRONT_AND_BACK, GL_AMBIENT, p->ambient.blend( p->alpha ) );
		glMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE, p->diffuse.blend( p->alpha ) );
		glMaterial( GL_FRONT_AND_BACK, GL_EMISSION, p->emissive.blend( p->alpha ) );
		
		if ( s && s->spec )
		{
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, p->shininess );
			glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, p->specular.blend( p->alpha ) );
		}
		else
		{
			glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 0.0 );
			glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, Color4( 0.0, 0.0, 0.0, p->alpha ) );
		}
	}
	else
	{
		Color4 a( 0.4, 0.4, 0.4, 1.0 );
		Color4 d( 0.8, 0.8, 0.8, 1.0 );
		Color4 s( 1.0, 1.0, 1.0, 1.0 );
		glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 33.0 );
		glMaterial( GL_FRONT_AND_BACK, GL_AMBIENT, a );
		glMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE, d );
		glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, s );
	}
}

void SpecularProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	if ( iBlock.isValid() && iBlock == block )
	{
		spec = nif->get<int>( iBlock, "Flags" ) != 0;
	}
}

void WireframeProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	if ( iBlock.isValid() && iBlock == block )
	{
		wire = nif->get<int>( iBlock, "Flags" ) != 0;
	}
}

void glProperty( WireframeProperty * p )
{
	if ( p && p->wire )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glLineWidth( 1.0 );
	}
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

void VertexColorProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	if ( iBlock.isValid() && iBlock == block )
	{
		vertexmode = nif->get<int>( iBlock, "Vertex Mode" );
		// 0 : source ignore
		// 1 : source emissive
		// 2 : source ambient + diffuse
		lightmode = nif->get<int>( iBlock, "Lighting Mode" );
		// 0 : emissive
		// 1 : emissive + ambient + diffuse

/*
vertex_mode 0
lighting_mode 0 : all black ?
lighting_mode 1 : material color (vertex color ignored)

vertex_mode 1
lighting_mode 0 : only vertex lighting (disable lighting?)
lighting_mode 1 : almost the same but the dark shadows dissapear when switching to bright light

vertex_mode 2
lighting_mode 0 : all black ?
lighting_mode 1 : the same as no property (additiv?)
*/

	}
}

void glProperty( VertexColorProperty * p, bool vertexcolors )
{
	if ( ! vertexcolors )
	{
		glDisable( GL_COLOR_MATERIAL );
		glColor( Color4( 1.0, 1.0, 1.0, 1.0 ) );
		return;
	}
	
	if ( p )
	{
		//if ( p->lightmode )
		{
			switch ( p->vertexmode )
			{
				case 0:
					glDisable( GL_COLOR_MATERIAL );
					glColor( Color4( 1.0, 1.0, 1.0, 1.0 ) );
					return;
				case 1:
					glEnable( GL_COLOR_MATERIAL );
					glColorMaterial( GL_FRONT_AND_BACK, GL_EMISSION );
					return;
				case 2:
				default:
					glEnable( GL_COLOR_MATERIAL );
					glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
					return;
			}
		}
		//else
		//{
		//	glDisable( GL_LIGHTING );
		//	glDisable( GL_COLOR_MATERIAL );
		//}
	}
	else
	{
		glEnable( GL_COLOR_MATERIAL );
		glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
	}
}

void StencilProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );
	if ( iBlock.isValid() && iBlock == block )
	{
		//static const GLenum functions[8] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
		//static const GLenum operations[8] = { GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT, GL_KEEP, GL_KEEP };
		
		// ! glFrontFace( GL_CCW )
		switch ( nif->get<int>( iBlock, "Draw Mode" ) )
		{
			case 2:
				cullEnable = true;
				cullMode = GL_FRONT;
				break;
			case 3:
				cullEnable = false;
				cullMode = GL_BACK;
				break;
			case 1:
			default:
				cullEnable = true;
				cullMode = GL_BACK;
				break;
		}
	}
}

void glProperty( StencilProperty * p )
{
	if ( p )
	{
		if ( p->cullEnable )
			glEnable( GL_CULL_FACE );
		else
			glDisable( GL_CULL_FACE );
		glCullFace( p->cullMode );
	}
	else
	{
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
	}
}
